\name{evalHermitePoly}
\alias{evalHermitePoly}
\title{
Evaluate Hermite polynomial at given location
}

\description{
Evaluate Hermite polynomial of given degree at given location.
This function is provided for demonstration/teaching purposes; this method is
not used by gaussHermiteData. It is numerically unstable for high-degree
polynomials.
}

\usage{
evalHermitePoly(x,n)
}

\arguments{
\item{x}{Vector of location(s) at which polynomial will be evaluated}
\item{n}{Degree of Hermite polynomial to compute}
}

\value{
Vector of length(x) values of Hermite polynomial
}

\author{Alexander W Blocker \email{ablocker@gmail.com}}

\seealso{
\code{\link{gaussHermiteData}}, \code{\link{aghQuad}}, \code{\link{ghQuad}}
}

\keyword{math}