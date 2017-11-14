//trigTable is a table with 10 rows for ten combinations or p^(mu)p_(nu) normalized by the energy
#pragma once
#include <stdio.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_sort_vector.h>
#include "Parameter.h"
#define PI 3.141592654f
#define REGULATE 1 // 1 to regulate dilute regions of space, sets energy density to zero if < tolerance

void calculateHypertrigTable(float ****hypertrigTable, parameters params)
{
  int DIM_RAP = params.DIM_RAP;
  int DIM_PHIP = params.DIM_PHIP;
  int DIM_ETA = params.DIM_ETA;
  float DRAP = params.DRAP;
  float DETA = params.DETA;
  float TAU = params.TAU;

  float rapmin = (-1.0) * ((float)(DIM_RAP-1) / 2.0) * DRAP;
  float etamin = (-1.0) * ((float)(DIM_ETA-1) / 2.0) * DETA;
  #pragma omp parallel for simd
  for (int irap = 0; irap < DIM_RAP; irap++)
  {
    float rap = (float)irap * DRAP + rapmin;

    for (int iphip = 0; iphip < DIM_PHIP; iphip++)
    {
      float phip = float(iphip) * (2.0 * PI) / float(DIM_PHIP);

      for (int ieta = 0; ieta < DIM_ETA; ieta++)
      {
        float eta = (float)ieta * DETA  + etamin;

        hypertrigTable[0][irap][iphip][ieta] = 1.0; //p^tau, p^tau component
        hypertrigTable[1][irap][iphip][ieta] = cos(phip) / cosh(rap - eta); //p^tau, p^x
        hypertrigTable[2][irap][iphip][ieta] = sin(phip) / cosh(rap - eta); //p^tau, p^y
        hypertrigTable[3][irap][iphip][ieta] = (-1.0 / TAU) * tanh(rap - eta); //p^tau, p^eta
        hypertrigTable[4][irap][iphip][ieta] = (cos(phip) * cos(phip)) / (cosh(rap - eta) * cosh(rap - eta)); //p^x, p^x
        hypertrigTable[5][irap][iphip][ieta] = (cos(phip) * sin(phip)) / (cosh(rap - eta) * cosh(rap - eta)); //p^x, p^y
        hypertrigTable[6][irap][iphip][ieta] = (-1.0 / TAU) * (cos(phip) * tanh(rap - eta)) / cosh(rap - eta); //p^x, p^eta
        hypertrigTable[7][irap][iphip][ieta] = (sin(phip) * sin(phip)) / (cosh(rap - eta) * cosh(rap - eta)); //p^y, p^y
        hypertrigTable[8][irap][iphip][ieta] = (-1.0 / TAU) * (sin(phip) * tanh(rap - eta)) / cosh(rap - eta); //p^y, p^eta
        hypertrigTable[9][irap][iphip][ieta] = (1.0 / (TAU * TAU)) * tanh(rap - eta) * tanh(rap - eta); //p^eta, p^eta
      }
    }
  }
}

void calculateStressTensor(float **stressTensor, float ***shiftedDensity, float ****hypertrigTable, parameters params)
{
  //int DIM_X = params.DIM_X;
  int DIM_Y = params.DIM_Y;
  int DIM_ETA = params.DIM_ETA;
  int DIM_RAP = params.DIM_RAP;
  int DIM_PHIP = params.DIM_PHIP;
  int DIM = params.DIM;
  float DRAP = params.DRAP;
  //float TAU = params.TAU;

  float d_phip = (2.0 * PI) / float(DIM_PHIP);

  for (int ivar = 0; ivar < 10; ivar++)
  {
    #pragma omp parallel for simd
    for (int is = 0; is < DIM; is++) //the column packed index for x, y and z
    {
      int ix = is / (DIM_Y * DIM_ETA);
      int iy = (is - (DIM_Y * DIM_ETA * ix))/ DIM_ETA;
      int ieta = is - (DIM_Y * DIM_ETA * ix) - (DIM_ETA * iy);

      for (int irap = 0; irap < DIM_RAP; irap++)
      {
        for (int iphip = 0; iphip < DIM_PHIP; iphip++)
        {
          //rather than gauss quadrature, just doing a elementary Riemann sum here; check convergence!
          // T^(mu,nu) = int deta int dphip G^(mu,nu)
          stressTensor[ivar][is] += shiftedDensity[is][irap][iphip] * hypertrigTable[ivar][irap][iphip][ieta];
        }
      }
      stressTensor[ivar][is] = stressTensor[ivar][is] * DRAP * d_phip; //multiply by common differential factor once
    }
  }
}

void calculateBaryonCurrent(float **baryonCurrent, float ***shiftedChargeDensity, float ****hypertrigTable, parameters params)
{
  //int DIM_X = params.DIM_X;
  int DIM_Y = params.DIM_Y;
  int DIM_ETA = params.DIM_ETA;
  int DIM_RAP = params.DIM_RAP;
  int DIM_PHIP = params.DIM_PHIP;
  int DIM = params.DIM;
  float DRAP = params.DRAP;

  float d_phip = (2.0 * PI) / float(DIM_PHIP);

  for (int ivar = 0; ivar < 4; ivar++)
  {
    #pragma omp parallel for simd
    for (int is = 0; is < DIM; is++) //the column packed index for x, y and z
    {
      int ix = is / (DIM_Y * DIM_ETA);
      int iy = (is - (DIM_Y * DIM_ETA * ix))/ DIM_ETA;
      int ieta = is - (DIM_Y * DIM_ETA * ix) - (DIM_ETA * iy);

      for (int irap = 0; irap < DIM_RAP; irap++)
      {
        for (int iphip = 0; iphip < DIM_PHIP; iphip++)
        {
          //rather than gauss quadrature, just doing a elementary Riemann sum here; check convergence!
          // T^(mu,nu) = int deta int dphip G^(mu,nu)
          baryonCurrent[ivar][is] += shiftedChargeDensity[is][irap][iphip] * hypertrigTable[ivar][irap][iphip][ieta];
        }
      }
      baryonCurrent[ivar][is] = baryonCurrent[ivar][is] * DRAP * d_phip; //multiply by common differential factor once
    }
  }
}

void solveEigenSystem(float **stressTensor, float *energyDensity, float **flowVelocity, parameters params)
{
  int DIM = params.DIM;
  float TAU = params.TAU;

  float tolerance = 1.0e18; //set quantities to zero which are less than 10^(-18) if REGULATE is true

  #pragma omp parallel for simd
  for (int is = 0; is < DIM; is++)
  {
    gsl_matrix *Tmunu; //T^(mu,nu) with two contravariant indices; we need to lower an index
    //using the metric to find the eigenvectors of T^(mu)_(nu) with one contravariant and one contravariant index
    Tmunu = gsl_matrix_alloc(4,4);
    gsl_matrix *gmunu;
    gmunu = gsl_matrix_alloc(4,4);
    gsl_matrix_complex *eigen_vectors;
    eigen_vectors = gsl_matrix_complex_alloc(4,4);
    gsl_vector_complex *eigen_values;
    eigen_values = gsl_vector_complex_alloc(4);
    //set the values of the energy momentum tensor
    gsl_matrix_set(Tmunu, 0, 0, stressTensor[0][is]); //tau,tau
    gsl_matrix_set(Tmunu, 0, 1, stressTensor[1][is]); //tau,x
    gsl_matrix_set(Tmunu, 0, 2, stressTensor[2][is]); //tau,y
    gsl_matrix_set(Tmunu, 0, 3, stressTensor[3][is]); //tau,eta
    gsl_matrix_set(Tmunu, 1, 1, stressTensor[4][is]); //x,x
    gsl_matrix_set(Tmunu, 1, 2, stressTensor[5][is]); //x,y
    gsl_matrix_set(Tmunu, 1, 3, stressTensor[6][is]); //x,eta
    gsl_matrix_set(Tmunu, 2, 2, stressTensor[7][is]); //y,y
    gsl_matrix_set(Tmunu, 2, 3, stressTensor[8][is]); //y,eta
    gsl_matrix_set(Tmunu, 3, 3, stressTensor[9][is]); //eta,eta
    gsl_matrix_set(Tmunu, 1, 0, stressTensor[1][is]); //x,tau
    gsl_matrix_set(Tmunu, 2, 0, stressTensor[2][is]); //y,tau
    gsl_matrix_set(Tmunu, 3, 0, stressTensor[3][is]); //eta,tau
    gsl_matrix_set(Tmunu, 2, 1, stressTensor[5][is]); //y,x
    gsl_matrix_set(Tmunu, 3, 1, stressTensor[6][is]); //eta,x
    gsl_matrix_set(Tmunu, 3, 2, stressTensor[8][is]); //eta,y

    //set the values of the "metric"; not really the metric, but the numerical constants
    //which are multiplied by the elements of T^(mu,nu) to get the values of T^(mu)_(nu)
    //note factors of TAU appropriate for milne coordinates g_(mu.nu) = diag(1,-1,-1,-TAU^2)
    gsl_matrix_set(gmunu, 0, 0, 1.0); //tau,tau
    gsl_matrix_set(gmunu, 0, 1, -1.0); //tau,x
    gsl_matrix_set(gmunu, 0, 2, -1.0); //tau,y
    gsl_matrix_set(gmunu, 0, 3, -1.0*TAU*TAU); //tau,eta
    gsl_matrix_set(gmunu, 1, 0, 1.0); //x,tau
    gsl_matrix_set(gmunu, 1, 1, -1.0); //x,x
    gsl_matrix_set(gmunu, 1, 2, -1.0); //x,y
    gsl_matrix_set(gmunu, 1, 3, -1.0*TAU*TAU); //x,eta
    gsl_matrix_set(gmunu, 2, 0, 1.0); //y,tau
    gsl_matrix_set(gmunu, 2, 1, -1.0); //y,x
    gsl_matrix_set(gmunu, 2, 2, -1.0); //y,y
    gsl_matrix_set(gmunu, 2, 3, -1.0*TAU*TAU); //y,eta
    gsl_matrix_set(gmunu, 3, 0, 1.0); //eta,tau
    gsl_matrix_set(gmunu, 3, 1, -1.0); //eta,x
    gsl_matrix_set(gmunu, 3, 2, -1.0); //eta,y
    gsl_matrix_set(gmunu, 3, 3, -1.0*TAU*TAU); //eta,eta
    //lower one index of the stress tensor; save it to the same matrix to save memory
    gsl_matrix_mul_elements(Tmunu, gmunu); //result stored in Tmunu !this multiplies element-wise, not ordinary matrix multiplication!
    gsl_eigen_nonsymmv_workspace *eigen_workspace;
    eigen_workspace = gsl_eigen_nonsymmv_alloc(4);
    gsl_eigen_nonsymmv(Tmunu, eigen_values, eigen_vectors, eigen_workspace);
    gsl_eigen_nonsymmv_free(eigen_workspace);

    //***does this have a solution for energy density and flow at every point?
    for (int i = 0; i < 4; i++)
    {
      gsl_complex eigenvalue = gsl_vector_complex_get(eigen_values, i);

      //this is the code that Jia Liu uses. Why does it work for him?
      if (GSL_REAL(eigenvalue) > 0.0 && GSL_IMAG(eigenvalue) == 0) //choose eigenvalue
      {
        gsl_complex v0 = gsl_matrix_complex_get(eigen_vectors, 0 , i);
        gsl_complex v1 = gsl_matrix_complex_get(eigen_vectors, 1 , i);
        gsl_complex v2 = gsl_matrix_complex_get(eigen_vectors, 2 , i);
        gsl_complex v3 = gsl_matrix_complex_get(eigen_vectors, 3 , i);

        if (GSL_IMAG(v0) == 0 && (2.0 * GSL_REAL(v0) * GSL_REAL(v0)-1) > 0) //choose eigenvector
        {
          //why does he use this as a scaling factor???
          //double factor = sqrt(1.0 / (3.0 * GSL_REAL(v0) * GSL_REAL(v0) - 1));
          //I think we should scale by minkowski length to get u^(mu)u_(mu) = 1
          double minkowskiLength = GSL_REAL(v0)*GSL_REAL(v0) - (GSL_REAL(v1)*GSL_REAL(v1) + GSL_REAL(v2)*GSL_REAL(v2) + TAU*TAU*GSL_REAL(v3)*GSL_REAL(v3));
          double factor = 1.0 / sqrt(minkowskiLength);

          if (GSL_REAL(v0) < 0) factor=-factor;

          energyDensity[is] = GSL_REAL(eigenvalue) / abs(factor);
          flowVelocity[0][is] = GSL_REAL(v0) * factor;
          flowVelocity[1][is] = GSL_REAL(v1) * factor;
          flowVelocity[2][is] = GSL_REAL(v2) * factor;
          flowVelocity[3][is] = GSL_REAL(v3) * factor;
        }
      }
      /*
      // begin new code that tries to select timelike eigenvector
      double v0 = GSL_REAL(gsl_matrix_complex_get(eigen_vectors, i , 0));
      double v1 = GSL_REAL(gsl_matrix_complex_get(eigen_vectors, i , 1));
      double v2 = GSL_REAL(gsl_matrix_complex_get(eigen_vectors, i , 2));
      double v3 = GSL_REAL(gsl_matrix_complex_get(eigen_vectors, i , 3));
      double minkowskiLength = v0*v0 - (v1*v1 + v2*v2 + TAU*TAU*v3*v3); //we want to flow velocity normalized s.t. minkowskiLength = 1
      double scaleFactor = 1.0 / sqrt(minkowskiLength); //so we need to scale all the elements of the eigenvector by scaleFactor
      //try selecting timelike eigenvector
      if (minkowskiLength > 0.0)
      {
        energyDensity[is] = GSL_REAL(eigenvalue) / scaleFactor; //do we need to scale the eigenvalue by the inverse of scaleFactor?
        //if (energyDensity[is] < 0.0) printf("negative energy density! e = %f\n", energyDensity[is]);
        flowVelocity[0][is] = v0 * scaleFactor;
        flowVelocity[1][is] = v1 * scaleFactor;
        flowVelocity[2][is] = v2 * scaleFactor;
        flowVelocity[3][is] = v3 * scaleFactor;

        if (REGULATE && (energyDensity[is] * tolerance < 1.0)) //regulate very dilute regions of space
        {
          energyDensity[is] = 0.0;
          flowVelocity[0][is] = 1.0;
          flowVelocity[1][is] = 0.0;
          flowVelocity[2][is] = 0.0;
          flowVelocity[3][is] = 0.0;
        }
      }
      //end new code
      */
      //begin old code that checks energy density rather than if flow is timelike
      /*
      if (GSL_REAL(eigenvalue) > 0.0 && GSL_IMAG(eigenvalue) == 0.0) //try checking if eigenvector is timelike instead! see above
      {
        double v0 = GSL_REAL(gsl_matrix_complex_get(eigen_vectors, i , 0));
        double v1 = GSL_REAL(gsl_matrix_complex_get(eigen_vectors, i , 1));
        double v2 = GSL_REAL(gsl_matrix_complex_get(eigen_vectors, i , 2));
        double v3 = GSL_REAL(gsl_matrix_complex_get(eigen_vectors, i , 3));
        //gsl normalizes eigenvectors to euclidean length = 1; we want a vector with minkowski length = 1, so we rescale
        //note factors of TAU appropriate for milne coordinates
        double minkowskiLength = v0*v0 - (v1*v1 + v2*v2 + TAU*TAU*v3*v3); //we want to flow velocity normalized s.t. minkowskiLength = 1
        double scaleFactor = 1.0 / sqrt(minkowskiLength); //so we need to scale all the elements of the eigenvector by scaleFactor
        //printf("scaled eigenvector %d is (%f ,%f , %f, %f) and eigenvalue %d is %f\n", i, v0, v1, v2, v3, i, GSL_REAL(eigenvalue));
        //set values of energy density and flow velocity
        if (scaleFactor != scaleFactor) printf("NANNY NAN!\n");
        if ((REGULATE) && (GSL_REAL(eigenvalue) * tolerance <= 1.0)) //regulate dilute regions - check if this helps or not
        {
          energyDensity[is] = 0.0;
          flowVelocity[0][is] = 1.0;
          flowVelocity[1][is] = 0.0;
          flowVelocity[2][is] = 0.0;
          flowVelocity[3][is] = 0.0;
        }
        energyDensity[is] = GSL_REAL(eigenvalue) / scaleFactor; //do we need to scale the eigenvalue by the inverse of scaleFactor?
        flowVelocity[0][is] = v0 * scaleFactor;
        flowVelocity[1][is] = v1 * scaleFactor;
        flowVelocity[2][is] = v2 * scaleFactor;
        flowVelocity[3][is] = v3 * scaleFactor;
      }
      */
      //ends old code
    }
  }
}
void calculateBulkPressure(float **stressTensor, float *energyDensity, float *pressure, float *bulkPressure, parameters params)
{
  int DIM = params.DIM;
  float TAU = params.TAU;
  #pragma omp parallel for simd
  for (int is = 0; is < DIM; is++)
  {
    // PI = -1/3 * (T^(mu)_(mu) - epsilon) - p
    // T^(mu)_(mu) = T^(0,0) - T^(1,1) - T^(2,2) - (TAU^2)T^(3,3)
    float a =  stressTensor[0][is] - stressTensor[4][is] - stressTensor[7][is] - TAU*TAU*stressTensor[9][is];
    bulkPressure[is] = (-1.0/3.0) * (a - energyDensity[is]) - pressure[is];
  }
}
void calculateShearViscTensor(float **stressTensor, float *energyDensity, float **flowVelocity, float *pressure, float *bulkPressure, float **shearTensor, parameters params)
{
  int DIM = params.DIM;
  float TAU = params.TAU;
  #pragma omp parallel for simd
  for (int is = 0; is < DIM; is++)
  {
    // pi^(mu,nu) = T^(mu,nu) - epsilon * u^(mu)u^(nu) + (P + PI) * (g^(mu,nu) - u^(mu)u^(nu))
    //calculate ten components : upper triangular part
    float b = energyDensity[is] + pressure[is] + bulkPressure[is];
    float c = pressure[is] + bulkPressure[is];
    shearTensor[0][is] = stressTensor[0][is] - flowVelocity[0][is] * flowVelocity[0][is] * b + c; //pi^(tau,tau)
    shearTensor[1][is] = stressTensor[1][is] - flowVelocity[0][is] * flowVelocity[1][is] * b; //pi^(tau,x)
    shearTensor[2][is] = stressTensor[2][is] - flowVelocity[0][is] * flowVelocity[2][is] * b; //pi^(tau,y)
    shearTensor[3][is] = stressTensor[3][is] - flowVelocity[0][is] * flowVelocity[3][is] * b; //pi^(tau,eta)
    shearTensor[4][is] = stressTensor[4][is] - flowVelocity[1][is] * flowVelocity[1][is] * b - c; //pi^(x,x)
    shearTensor[5][is] = stressTensor[5][is] - flowVelocity[1][is] * flowVelocity[2][is] * b; //pi^(x,y)
    shearTensor[6][is] = stressTensor[6][is] - flowVelocity[1][is] * flowVelocity[3][is] * b; //pi^(x,eta)
    shearTensor[7][is] = stressTensor[7][is] - flowVelocity[2][is] * flowVelocity[2][is] * b - c; //pi^(y,y)
    shearTensor[8][is] = stressTensor[8][is] - flowVelocity[2][is] * flowVelocity[3][is] * b; //pi^(y,eta)
    shearTensor[9][is] = stressTensor[9][is] - flowVelocity[3][is] * flowVelocity[3][is] * b - c * (1.0/(TAU*TAU)); //pi^(eta,eta)
  }
}

// n_B = u^(mu)j_(mu)
void calculateBaryonDensity(float *baryonDensity, float **baryonCurrent, float **flowVelocity, parameters params)
{
  int DIM = params.DIM;
  float TAU = params.TAU;
  #pragma omp parallel for simd
  for (int is = 0; is < DIM; is++)
  {
    baryonDensity[is] = (flowVelocity[0][is] * baryonCurrent[0][is]) - (flowVelocity[1][is] * baryonCurrent[1][is]) - (flowVelocity[2][is] * baryonCurrent[2][is]) - (TAU * TAU * flowVelocity[3][is] * baryonCurrent[3][is]);
  }
}
// V^(mu) = j^(mu) - n_B * u^(mu)
void calculateBaryonDiffusion(float **baryonDiffusion, float **baryonCurrent, float *baryonDensity, float **flowVelocity, parameters params)
{
  int DIM = params.DIM;
  for (int ivar = 0; ivar < 4; ivar++)
  {
    #pragma omp parallel for simd
    for (int is = 0; is < DIM; is++)
    {
      baryonDiffusion[ivar][is] = baryonCurrent[ivar][is] - (baryonDensity[is] * flowVelocity[ivar][is]);
    }
  }
}
