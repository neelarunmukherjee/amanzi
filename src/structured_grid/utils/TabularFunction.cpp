#include "TabularFunction.H"

TabularFunction::TabularFunction(const Array<double>&      x,
                                 const Array<double>&      y,
                                 const Array<std::string>& form)
  : x_(x), y_(y)
{
  if (x.size() != y.size()) {
      std::cerr << "the number of x and y values differ" << std::endl;
    BoxLib::Abort();
  }
  for (int j = 1; j < x.size(); ++j) {
    if (x[j] <= x[j-1]) {
      std::cerr << "x values are not strictly increasing" << std::endl;
      BoxLib::Abort();
    }
  }
  if (form.size() != x.size()-1) {
    std::cerr << "incorrect number of form values specified" << std::endl;
    BoxLib::Abort();
    for (int j = 0; j < form.size(); ++j) {
        if (form[j]=="LINEAR") {
            form_[j] = LINEAR;
        }
        else if (form[j]=="CONSTANT") {
            form_[j] = CONSTANT;
        }
        else {
            std::string m = "Unsupported form type: ";
            m += form[j];
            BoxLib::Abort(m.c_str());
        }
    }
  }
}

Real
TabularFunction::operator() (Real x) const
{
    Real y;
    int n = x_.size();
    if (x <= x_[0]) {
    y = y_[0];
  } else if (x >= x_[n-1]) {
    y = y_[n-1];
  } else {
    // binary search to find interval containing x
    int j1 = 0, j2 = n-1;
    while (j2 - j1 > 1) {
      int j = (j1 + j2) / 2;
      if (x >= x_[j]) {
        j1 = j;
      } else {
        j2 = j;
      }
    }
    // Now have x_[j1] <= *x < x_[j2]
    switch (form_[j1]) {
    case LINEAR:
      // Linear interpolation between x[j1] and x[j2]
      y = y_[j1] + ((y_[j2]-y_[j1])/(x_[j2]-x_[j1])) * (x - x_[j1]);
      break;
    case CONSTANT:
      y = y_[j1];
      break;
    }
  }
  return y;
}

