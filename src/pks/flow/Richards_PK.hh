/*
  This is the flow component of the Amanzi code. 

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Neil Carlson (version 1) 
           Konstantin Lipnikov (version 2) (lipnikov@lanl.gov)
*/

#ifndef AMANZI_RICHARDS_PK_HH_
#define AMANZI_RICHARDS_PK_HH_

#include "Epetra_Vector.h"
#include "Epetra_IntVector.h"
#include "Epetra_Import.h"

#include "Teuchos_ParameterList.hpp"
#include "Teuchos_RCP.hpp"

#include "BCs.hh"
#include "BDF1_TI.hh"
#include "OperatorDiffusion.hh"
#include "OperatorAccumulation.hh"
#include "Upwind.hh"

#include "Flow_PK.hh"
#include "RelativePermeability.hh"
#include "TI_Specs.hh"

namespace Amanzi {
namespace Flow {

class Richards_PK : public Flow_PK {
 public:
  Richards_PK(const Teuchos::RCP<Teuchos::ParameterList>& global_list,
              const std::string& pk_list_name, Teuchos::RCP<State> S);
  ~Richards_PK();

  // main PK methods
  void Initialize(const Teuchos::Ptr<State>& S);
  void SetState(const Teuchos::RCP<State>& S) { S_ = S; }
  bool Advance(double dT_MPC, double& dT_actual); 
  double get_dt();
  void set_dt(double dt){dT = dt; dT_desirable_ = dT;}
  void CommitState(double dt, const Teuchos::Ptr<State>& S);
  void CalculateDiagnostics(const Teuchos::Ptr<State>& S);

  // main flow methods
  void InitSteadyState(double T0, double dT0);
  void InitTransient(double T0, double dT0);
  void InitPicard(double T0);
  void InitNextTI(double T0, double dT0, TI_Specs& ti_specs);
  void InitTimeInterval();

  int AdvanceToSteadyState(double T0, double dT0);
  void InitializeAuxiliaryData();
  void InitializeSteadySaturated();

  int AdvanceToSteadyState_Picard(TI_Specs& ti_specs);
  int AdvanceToSteadyState_BackwardEuler(TI_Specs& ti_specs);
  int AdvanceToSteadyState_BDF1(TI_Specs& ti_specs);

  // methods for experimental time integration
  double ErrorNormSTOMP(const CompositeVector& u, const CompositeVector& du);
  double ErrorNormPicardExperimental(const CompositeVector& uold, const CompositeVector& unew);
 
  // methods required for time integration
  void Functional(const double T0, double T1, 
                  Teuchos::RCP<CompositeVector> u_old, Teuchos::RCP<CompositeVector> u_new, 
                  Teuchos::RCP<CompositeVector> f);
  void ApplyPreconditioner(Teuchos::RCP<const CompositeVector> u, Teuchos::RCP<CompositeVector> Hu);
  void UpdatePreconditioner(double T, Teuchos::RCP<const CompositeVector> u, double dT);
  double ErrorNorm(Teuchos::RCP<const CompositeVector> u, Teuchos::RCP<const CompositeVector> du);
  bool IsAdmissible(Teuchos::RCP<const CompositeVector> up) { return true; }
  bool ModifyPredictor(double dT, Teuchos::RCP<const CompositeVector> u0,
                       Teuchos::RCP<CompositeVector> u);
  AmanziSolvers::FnBaseDefs::ModifyCorrectionResult
      ModifyCorrection(double dT, Teuchos::RCP<const CompositeVector> res,
                       Teuchos::RCP<const CompositeVector> u, 
                       Teuchos::RCP<CompositeVector> du);
  void ChangedSolution() {};

  // other main methods
  double ComputeUDot(double T, const Epetra_Vector& u, Epetra_Vector& udot);
  void UpdateSourceBoundaryData(double T0, double T1, const CompositeVector& u);

  // linear problems and solvers
  void SolveFullySaturatedProblem(double T0, CompositeVector& u, const std::string& solver_name);
  void EnforceConstraints(double T1, CompositeVector& u);

  // water retention models
  void DeriveSaturationFromPressure(const Epetra_MultiVector& p, Epetra_MultiVector& s);
  void DerivePressureFromSaturation(const Epetra_MultiVector& s, Epetra_MultiVector& p);

  // initization members
  void ClipHydrostaticPressure(const double pmin, Epetra_MultiVector& p);
  void ClipHydrostaticPressure(const double pmin, const double s0, Epetra_MultiVector& p);

  double CalculateRelaxationFactor(const Epetra_MultiVector& uold,
                                   const Epetra_MultiVector& unew);

  // control method
  void ResetParameterList(const Teuchos::ParameterList& rp_list_new) { rp_list_ = rp_list_new; }
  
  // access methods
  Teuchos::RCP<Operators::Operator> op_matrix() { return op_matrix_; }
  const Teuchos::RCP<CompositeVector> get_solution() { return solution; }

  // developement members
  void ImproveAlgebraicConsistency(const Epetra_Vector& ws_prev, Epetra_Vector& ws);

  template <class Model> 
  double DeriveBoundaryFaceValue(int f, const CompositeVector& u,
          const Model& model);
  virtual double BoundaryFaceValue(int f, const CompositeVector& pressure);
  
 public:
  Teuchos::ParameterList rp_list_;

 private:
  Teuchos::RCP<RelativePermeability> rel_perm_;
  Teuchos::RCP<Operators::Operator> op_matrix_, op_preconditioner_;
  Teuchos::RCP<Operators::OperatorDiffusion> op_matrix_diff_, op_preconditioner_diff_;
  Teuchos::RCP<Operators::OperatorAccumulation> op_acc_;
  Teuchos::RCP<Operators::Upwind<RelativePermeability> > upwind_;
  Teuchos::RCP<Operators::BCs> op_bc_;

  Teuchos::RCP<BDF1_TI<CompositeVector, CompositeVectorSpace> > bdf1_dae;  // Time integrators
  int block_picard;

  int error_control_;
  double dT_desirable_;

  double functional_max_norm;
  int functional_max_cell;

  Teuchos::RCP<CompositeVector> solution;  // copies of state variables
  Teuchos::RCP<CompositeVector> darcy_flux_copy;

  Teuchos::RCP<Epetra_Vector> pdot_cells_prev;  // time derivative of pressure
  Teuchos::RCP<Epetra_Vector> pdot_cells;

  int update_upwind;
  Teuchos::RCP<CompositeVector> darcy_flux_upwind;  // used in  

 private:
  void operator=(const Richards_PK& RPK);

  friend class Richards_PK_Wrapper;
};


/* ******************************************************************
* Calculates solution value on the boundary.
****************************************************************** */
template <class Model> 
double Richards_PK::DeriveBoundaryFaceValue(
    int f, const CompositeVector& u, const Model& model) 
{
  if (u.HasComponent("face")) {
    const Epetra_MultiVector& u_face = *u.ViewComponent("face");
    return u_face[f][0];
  } else {
    const std::vector<int>& bc_model = op_bc_->bc_model();
    const std::vector<double>& bc_value = op_bc_->bc_value();

    if (bc_model[f] == Operators::OPERATOR_BC_DIRICHLET) {
      return bc_value[f];
    } else {
      const Epetra_MultiVector& u_cell = *u.ViewComponent("cell");
      AmanziMesh::Entity_ID_List cells;
      mesh_->face_get_cells(f, AmanziMesh::USED, &cells);
      int c = cells[0];
      return u_cell[0][c];
    }
  }
}



}  // namespace Flow
}  // namespace Amanzi

#endif

