#include "lib.h"

void buildHermiteJacobi( int n, double* D, double* E ) {
//
// Construct symmetric tridiagonal matrix similar to Jacobi matrix
// for Hermite polynomials
//
// On exit, D contains diagonal elements of said matrix;
// E contains subdiagonal elements.
//
// Need D of size n, E of size n-1
//
// Building matrix based on recursion relation for monic versions of Hermite
// polynomials:
//      p_n(x) = H_n(x) / 2^n
//      p_n+1(x) + (B_n-x)*p_n(x) + A_n*p_n-1(x) = 0
//      B_n = 0
//      A_n = n/2
// 
// Matrix similar to Jacobi (J) defined by:
//      J_i,i = B_i-1, i = 1, ..., n
//      J_i,i-1 = J_i-1,i = sqrt(A_i-1), i = 2, ..., n
//

    // Build diagonal
    int i;
    for (i=0; i<n; i++) {
        D[i]    = 0;
    }

    // Build sub/super-diagonal
    for (i=0; i<n-1; i++) {
        E[i]    = sqrt((i+1.0)/2);
    }
    
}

void quadInfoGolubWelsch( int n, double* D, double* E, double mu0,
                          double* x, double* w ) {
//
// Compute weights & nodes for Gaussian quadrature using Golub-Welsch algorithm.
//
// First need to build symmetric tridiagonal matrix J similar to Jacobi for
// desired orthogonal polynomial (based on recurrence relation).
// 
// D contains the diagonal of this matrix J, and E contains the
// sub/super-diagonal.
//
// This routine finds the eigenvectors & values of the given J matrix.
//
// The eigenvalues correspond to the nodes for the desired quadrature rule
// (roots of the orthogonal polynomial).
//
// The eigenvectors can be used to compute the weights for the quadrature rule
// via:
//
//      w_j = mu0 * (v_j,1)^2
// 
// where mu0 = \int_a^b w(x) dx
// (integral over range of integration of weight function)
//
// and 
//
// v_j,1 is the first entry of the jth normalized (to unit length) eigenvector.
//
// On exit, x (length n) contains nodes for quadrature rule, and w (length n)
// contains weights for quadrature rule.
//
// Note that contents of D & E are destroyed on exit
//

    // Setup for eigenvalue computations
    char JOBZ   = 'V'; // Compute eigenvalues & vectors
    int INFO;

    // Initialize array for workspace
    double * WORK   = new double[2*n-2];

    // Initialize array for eigenvectors
    double * Z      = new double[n*n];

    // Run eigen decomposition
    F77_NAME(dstev)(&JOBZ, &n, D, E,     // Job flag & input matrix
            Z, &n,              // Output array for eigenvectors & dim
            WORK, &INFO         // Workspace & info flag
            );

    // Setup x & w
    int i;
    for (i=0; i<n; i++) {
        x[i] = D[i];
        w[i] = mu0*Z[i*n]*Z[i*n];
    }

    // Deallocate temporary arrays
    delete WORK;
    delete Z;
}

void findPolyRoots( double* c, int n, double* r ) {
//
// Compute roots of polynomial with coefficients c using eigenvalue
// decomposition of companion matrix
//
// Using R LAPACK interface
//
// Places result into r, which needs to be of dimension n-1
// Need c of dimension n
//
    using namespace std;
    
    int i, j;

    // Build companion matrix; column-major order for compatibility with LAPACK
    double * C  = new double[n*n];
    for (i=0; i<n*n; i++) {
        C[i] = 0;
    }
    
    // Add diagonal components
    for (i=1; i<n; i++) {
        C[i+n*(i-1)] = 1;
    }

    // Add coefficients
    for (i=0; i<n; i++) {
        C[i+n*(n-1)] = -c[i]/c[n];
    }

    /*
    // Debug code; print matrix
    for (i=0; i<n; i++) {
        for (j=0; j<n; j++) {
            fprintf(stdout, "%0.2g\t", C[i+n*j]);
        }
        fprintf(stdout, "\n");
    }
    */

    // Setup for eigenvalue computation
    
    // Allocate vectors for real & imaginary parts of eigenvalues
    double * valI = new double[n];

    // Integers for status codes and LWORK
    int INFO, LWORK;

    // Workspace; starting as a single double
    double tmpwork;
    double * work;

    // Run eigenvalue computation
    char no = 'N';
    int one = 1;
    //
    // First, get optimal workspace size
    LWORK = -1;
    F77_CALL(dgeev)(  &no, &no,   // Don't compute eigenvectors
            &n, C, &n,  // Companion matrix & dimensions; overwritten on exit
            r, valI,    // Arrays for real & imaginary parts of eigenvalues
            NULL, &one, // VL & LDVL; not used
            NULL, &one, // VR & LDVR; not used
            &tmpwork,   // Workspace; will contain optimal size upon exit
            &LWORK,     // Workspace size; -1 -> get optimal size
            &INFO       // Status code
            );
    
    // Next, actually run procedure
    LWORK   = (int) tmpwork;
    work    = new double[LWORK];
    F77_CALL(dgeev)(&no, &no,   // Don't compute eigenvectors
            &n, C, &n,  // Companion matrix & dimensions; overwritten on exit
            r, valI,    // Arrays for real & imaginary parts of eigenvalues
            NULL, &one, // VL & LDVL; not used
            NULL, &one, // VR & LDVR; not used
            work,      // Workspace; will contain optimal size upon exit
            &LWORK,     // Workspace size; -1 -> get optimal size
            &INFO       // Status code
            );

    // Deallocate temporary arrays
    delete[] valI;
    delete[] C;
    delete[] work;
}

SEXP findPolyRoots( SEXP cR ) {
    using namespace Rcpp;

    // Convert coef to Rcpp object
    NumericVector c(cR);
    int n = c.size();

    // Allocate vector for results
    NumericVector roots(n-1,0.0);
    
    // Compute roots
    findPolyRoots(&c[0], n-1, &roots[0]);
    
    return roots;
}

void hermitePolyCoef( int n, double* c ) {
//
// Compute coefficients of Hermite polynomial of order n
// Need c of dimension n+1
//
// Uses recursion relation for efficiency

    // Allocate workspace for coefficient evaluations;
    // will use column-major ordering
    long * work = new long[(n+1)*(n+1)];
    int i, j;
    for (i=0; i<(n+1)*(n+1); i++) {
        work[i] = 0;
    }

    // Handle special cases (n<2)
    if (n==0) {
        c[0]    = 1;
        return;
    }
    else if (n==1) {
        c[0]    = 0;
        c[1]    = 2;
        return;
    }

    // Initialize recursion
    work[0]   = 1;    // H_0(x) = 1
    work[1]   = 0;
    work[1+1*(n+1)]   = 2;    // H_1(x) = 2*x

    // Run recursion relation
    for(i=2; i<n+1; i++) {
        // Order 0 term updates
        work[i] = -2*(i-1)*work[i-2];
        for(j=1; j<i+1; j++) {
            // Remainder of recursion relation
            work[i+j*(n+1)] = 2*work[(i-1)+(j-1)*(n+1)] -
                                2*(i-1)*work[(i-2)+j*(n+1)];
        }
    }
    
    // Extract double-formatted coefficients from last row
    for (j=0; j<n+1; j++) {
        c[j] = (double) work[n+j*(n+1)];
    }

    // Deallocate workspace
    delete work;
}

SEXP hermitePolyCoef( SEXP nR ) {
    using namespace Rcpp;

    // Convert coef to Rcpp object
    int n = IntegerVector(nR)[0];

    // Allocate vector for coefficients
    NumericVector coef(n+1,0.0);
    
    // Compute roots
    hermitePolyCoef(n, &coef[0]);
    
    return coef;
}

double hermitePoly( double x, int n ) {
//
// Compute Hermite polynomial of order n evaluated at x efficiently via
// recursion relation:
//      H_n+1(x)    = 2*x*H_n(x) - 2*n*H_n-1(x)
//      H_0(x)      = 1
//      H_1(x)      = 2x
//
    int i = 0;

    // Special cases
    if (n==0) {
        return 1;
    }
    else if (n==1) {
        return 2*x;
    }

    // Standard recursion
    double hnm2 = 1;
    double hnm1 = 2*x;
    double hn;
    for (i=2; i<=n; i++) {
        hn      = 2*x*hnm1 - 2*(i-1)*hnm2;
        hnm2    = hnm1;
        hnm1    = hn;
    }
    
    return hn;
}

SEXP evalHermitePoly(SEXP xR, SEXP nR) {
    using namespace Rcpp;
    int i;

    // Convert to Rcpp objects
    NumericVector x(xR);
    IntegerVector n(nR);

    if (n.size()==x.size()) {
        // Iterate through x & n
        NumericVector h(x.size());
        
        for (i=0; i<x.size(); i++) {
            h[i]    = hermitePoly( x[i], n[i] );
        }

        return h;
    }
    else if (x.size() > n.size()) {
        // Iterate through x only
        NumericVector h(x.size());
        
        for (i=0; i<x.size(); i++) {
            h[i]    = hermitePoly( x[i], n[0] );
        }

        return h;
    }
    else {
        // Iterate through n only
        NumericVector h(n.size());
        
        for (i=0; i<n.size(); i++) {
            h[i]    = hermitePoly( x[0], n[i] );
        }

        return h;
    }
}

int gaussHermiteDataDirect( int n, double* x, double* w ) {
//
// Calculates roots & weights of Hermite polynomials of order n for
// Gauss-Hermite integration.
//
// Need x & w of size n
//
// Using standard formulation (no generalizations or polynomial adjustment)
// 
// Direct evaluation and root-finding; clear, but numerically unstable
// for n>20 or so
//
    // Calculate coefficients of Hermite polynomial of given order
    double * coef   = new double[n+1];
    hermitePolyCoef(n, coef);

    // Find roots of given Hermite polynomial; these are the points at
    // which the integrand will be evaluated (x)
    findPolyRoots(coef, n, x);

    // Calculate weights w
    int i;
    double log2 = log(2.0), logsqrtpi = 0.5*log(PI);
    for (i=0; i<n; i++) {
        // First, compute the log-weight
        w[i]    = (n-1)*log2 + lgamma(n+1) + logsqrtpi -
                  2*log((double)n) - 2*log(abs(hermitePoly(x[i], n-1)));
        w[i]    = exp(w[i]);
    }

    // Deallocate temporary objects
    delete coef;

    return 0;
}

int gaussHermiteData( int n, double* x, double* w ) {
//
// Calculates nodes & weights for Gauss-Hermite integration of order n
//
// Need x & w of size n
//
// Using standard formulation (no generalizations or polynomial adjustment)
//
// Evaluations use Golub-Welsch algorithm; numerically stable for n>=100
//
    // Build Jacobi-similar symmetric tridiagonal matrix via diagonal &
    // sub-diagonal
    double * D, * E;
    D   = new double[n];
    E   = new double[n];
    //
    buildHermiteJacobi( n, D, E );

    // Get nodes & weights
    double mu0  = sqrt(PI);
    quadInfoGolubWelsch( n, D, E, mu0,
                          x, w );

    // Deallocate temporary objects
    delete D;
    delete E;

    return 0;
}

SEXP gaussHermiteData( SEXP nR ) {
    using namespace Rcpp;

    // Convert nR to int
    int n   = IntegerVector(nR)[0];

    // Allocate vectors for x & w
    NumericVector x(n);
    NumericVector w(n);

    // Build Gauss-Hermite integration rules
    gaussHermiteData(n, &x[0], &w[0]);

    // Build list for values
    List ret;
    ret["x"] = x;
    ret["w"] = w;
    return ret;
}
