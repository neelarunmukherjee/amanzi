/*
  This is the operators component of the Amanzi code.

  License: BSD
  Authors: Konstantin Lipnikov (lipnikov@lanl.gov)

  Discrete source operator.
*/

#include "AnalyticBase.hh"

class Analytic01 : public AnalyticBase {
 public:
  Analytic01(Teuchos::RCP<const Amanzi::AmanziMesh::Mesh> mesh) : AnalyticBase(mesh) {};
  ~Analytic01() {};

  Amanzi::WhetStone::Tensor Tensor(const Amanzi::AmanziGeometry::Point& p, double t) {
    Amanzi::WhetStone::Tensor K(2, 2);
    K(0, 0) = Kxx_(p, t);
    K(1, 1) = Kyy_(p, t);
    K(0, 1) = Kxy_(p, t);
    K(1, 0) = Kxy_(p, t);

    return K;
  }

  double pressure_exact(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];
    double xy = x * y;
    return x * xy * xy + x * sin(2 * M_PI * xy) * sin(2 * M_PI * y);
  }

  Amanzi::AmanziGeometry::Point velocity_exact(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];

    double t01, t02, t03, t12, t13, t04, t05, t06; 
    double px, py;

    t01 = x*x*y;
    t02 = sin(2*M_PI*x*y);
    t03 = sin(2*M_PI*y);

    t12 = cos(2*M_PI*x*y);
    t13 = cos(2*M_PI*y);

    px = 3*y*t01 + t03*(t02 + 2*M_PI*y*x*t12);
    py = 2*x*t01 + x*2*M_PI*(x*t12*t03 + t02*t13);

    t04 = Kxx_(p, t);
    t05 = Kxy_(p, t);
    t06 = Kyy_(p, t);

    Amanzi::AmanziGeometry::Point v(2);
    v[0] = -(t04 * px + t05 * py);
    v[1] = -(t05 * px + t06 * py);
    return v;
  }

  Amanzi::AmanziGeometry::Point gradient_exact(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];

    double t01, t02, t03, t12, t13, t04, t05, t06; 
    double px, py;

    t01 = x*x*y;
    t02 = sin(2*M_PI*x*y);
    t03 = sin(2*M_PI*y);

    t12 = cos(2*M_PI*x*y);
    t13 = cos(2*M_PI*y);

    px = 3*y*t01 + t03*(t02 + 2*M_PI*y*x*t12);
    py = 2*x*t01 + x*2*M_PI*(x*t12*t03 + t02*t13);

    Amanzi::AmanziGeometry::Point v(2);
    v[0] = px;
    v[1] = py;
    return v;
  }

  double source_exact(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];

    double t01, t02, t03, t12, t13;
    double px, py, pxx, pxy, pyy;
    double t04, t05, t06, tx4, ty4, tx5, ty5, tx6;

    t01 = x*x*y;
    t02 = sin(2*M_PI*x*y);
    t03 = sin(2*M_PI*y);

    t12 = cos(2*M_PI*x*y);
    t13 = cos(2*M_PI*y);

    px = 3*y*t01 + t03*(t02 + 2*M_PI*y*x*t12);
    py = 2*x*t01 + x*2*M_PI*(x*t12*t03 + t02*t13);

    pxx = 6*x*y*y + 4*M_PI*t03*(y*t12 - M_PI*y*y*x*t02); 
    pxy = 6*x*x*y + 2*M_PI*(t13*t02 + 2*x*t03*t12 + x*y*2*M_PI*(t13*t12-x*t03*t02));
    pyy = 2*x*x*x + x*4*M_PI*M_PI*(-x*x*t02*t03 + 2*x*t12*t13 - t02*t03);

    t04 = Kxx_(p, t);
    t05 = Kxy_(p, t);
    t06 = Kyy_(p, t);

    tx4 = 2*(x+1);  // d/dx (Kxx)
    ty4 = 2*y;      // d/dy (Kxx)

    tx5 = -y;  // d/dx (Kxy)  
    ty5 = -x;  // d/dy (Kxy)
  
    tx6 = 2*(x+1);  // d/dy (Kxy)
    return -(tx4 + ty5)*px - tx5*py - t04*pxx - 2*t05*pxy - t06*pyy;
  }

 private:
  double Kxx_(const Amanzi::AmanziGeometry::Point& p, double t) {
    double x = p[0];
    double y = p[1]; 
    return (x + 1) * (x + 1) + y * y;
  }
  double Kyy_(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];
    return (x + 1) * (x + 1);
  }
  double Kxy_(const Amanzi::AmanziGeometry::Point& p, double t) { 
    double x = p[0];
    double y = p[1];
    return -x * y;
  }
};
