/*
  This is the energy component of the Amanzi code. 
  This is a base class for energy equations.

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#ifndef AMANZI_ENERGY_PK_HH_
#define AMANZI_ENERGY_PK_HH_

// TPLs
#include "Epetra_Vector.h"
#include "Epetra_FECrsMatrix.h"
#include "Teuchos_RCP.hpp"

// Amanzi
#include "CompositeVector.hh"
#include "Diffusion.hh"
#include "Operator.hh"
#include "OperatorAccumulation.hh"
#include "OperatorAdvection.hh"
#include "PK.hh"
#include "PK_BDF.hh"
#include "PK_DomainFunction.hh"
#include "PK_PhysicalBDF.hh"
#include "primary_variable_field_evaluator.hh"
#include "Tensor.hh"
#include "TreeVector.hh"
#include "VerboseObject.hh"


namespace Amanzi {
namespace Energy {

class Energy_PK : public PK_PhysicalBDF {
 public:
  Energy_PK(const Teuchos::RCP<Teuchos::ParameterList>& glist, Teuchos::RCP<State> S);
  virtual ~Energy_PK() {};

  // methods required by PK interface
  virtual void Setup(const Teuchos::Ptr<State>& S);
  virtual void Initialize(const Teuchos::Ptr<State>& S);
  virtual std::string name() { return passwd_; }

  // methods required for time integration
  // -- management of the preconditioner
  virtual int ApplyPreconditioner(Teuchos::RCP<const TreeVector> u, Teuchos::RCP<TreeVector> hu) {
    return op_preconditioner_->ApplyInverse(*u->Data(), *hu->Data());
  }

  // -- check the admissibility of a solution
  //    override with the actual admissibility check
  bool IsAdmissible(Teuchos::RCP<const TreeVector> up) {
    return true;
  }

  // -- possibly modifies the predictor that is going to be used as a
  //    starting value for the nonlinear solve in the time integrator,
  //    the time integrator will pass the predictor that is computed
  //    using extrapolation and the time step that is used to compute
  //    this predictor this function returns true if the predictor was
  //    modified, false if not
  bool ModifyPredictor(double dt, Teuchos::RCP<const TreeVector> u0, Teuchos::RCP<TreeVector> u) {
    return false;
  }

  // -- possibly modifies the correction, after the nonlinear solver (NKA)
  //    has computed it, will return true if it did change the correction,
  //    so that the nonlinear iteration can store the modified correction
  //    and pass it to NKA so that the NKA space can be updated
  AmanziSolvers::FnBaseDefs::ModifyCorrectionResult
      ModifyCorrection(double dt, Teuchos::RCP<const TreeVector> res,
                       Teuchos::RCP<const TreeVector> u,
                       Teuchos::RCP<TreeVector> du) {
    return AmanziSolvers::FnBaseDefs::CORRECTION_NOT_MODIFIED;
  }

  // -- calling this indicates that the time integration
  //    scheme is changing the value of the solution in state.
  void ChangedSolution() {
    temperature_eval_->SetFieldAsChanged(S_.ptr());
  }

  // other methods
  bool UpdateConductivityData(const Teuchos::Ptr<State>& S);
  void UpdateSourceBoundaryData(double T0, double T1, const CompositeVector& u);
  void ComputeBCs(const CompositeVector& u);

  // access for unit tests
  std::vector<WhetStone::Tensor>& get_K() { return K; } 
  Teuchos::RCP<PrimaryVariableFieldEvaluator>& temperature_eval() { return temperature_eval_; }

 private:
  void InitializeFields_();

 public:
  int ncells_owned, ncells_wghost;
  int nfaces_owned, nfaces_wghost;

 protected:
  Teuchos::RCP<const AmanziMesh::Mesh> mesh_;
  int dim;

  const Teuchos::RCP<Teuchos::ParameterList> glist_;
  Teuchos::RCP<Teuchos::ParameterList> ep_list_;
  Teuchos::RCP<const Teuchos::ParameterList> preconditioner_list_;
  Teuchos::RCP<Teuchos::ParameterList> ti_list_;

  // state and primary field
  Teuchos::RCP<State> S_;
  std::string passwd_;
  Teuchos::RCP<PrimaryVariableFieldEvaluator> temperature_eval_;

  // keys
  Key energy_key_, prev_energy_key_;
  Key enthalpy_key_;
  Key conductivity_key_;

  // conductivity tensor
  std::vector<WhetStone::Tensor> K; 

  // boundary conditons
  std::vector<Teuchos::RCP<PK_DomainFunction> > bc_temperature_; 
  std::vector<Teuchos::RCP<PK_DomainFunction> > bc_flux_; 

  std::vector<int> bc_model_; 
  std::vector<double> bc_value_, bc_mixed_; 
  int dirichlet_bc_faces_;

  // operators and solvers
  Teuchos::RCP<Operators::Diffusion> op_matrix_diff_, op_preconditioner_diff_;
  Teuchos::RCP<Operators::OperatorAccumulation> op_acc_;
  Teuchos::RCP<Operators::OperatorAdvection> op_matrix_advection_, op_preconditioner_advection_;
  Teuchos::RCP<Operators::Operator> op_matrix_, op_preconditioner_, op_advection_;
  Teuchos::RCP<Operators::BCs> op_bc_;

  std::string preconditioner_name_;
  bool prec_include_enthalpy_;

 protected:
  Teuchos::RCP<VerboseObject> vo_;
};

}  // namespace Energy
}  // namespace Amanzi

#endif
