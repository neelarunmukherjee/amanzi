/*
  Operators

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Authors: Konstantin Lipnikov (lipnikov@lanl.gov)
           Ethan Coon (ecoon@lanl.gov)

  This operator is a collection of local "DIAGONAL" Ops.
*/

#include "Accumulation.hh"
#include "Operator_Cell.hh"
#include "Operator_Edge.hh"
#include "Operator_Node.hh"
#include "Op_Cell_Cell.hh"
#include "Op_Edge_Edge.hh"
#include "Op_Node_Node.hh"
#include "Op_SurfaceCell_SurfaceCell.hh"

namespace Amanzi {
namespace Operators {

/* ******************************************************************
* Modifier for diagonal operators.  Op += du * vol / dt.
****************************************************************** */
void Accumulation::AddAccumulationTerm(
    const CompositeVector& du, double dT, const std::string& name)
{
  Teuchos::RCP<Op> op = FindOp_(name);
  Epetra_MultiVector& diag = *op->diag;

  CompositeVector vol(du);
  CalculateEntitylVolume_(vol, name);

  const Epetra_MultiVector& duc = *du.ViewComponent(name);
  Epetra_MultiVector& volc = *vol.ViewComponent(name); 

  int n = duc.MyLength();
  int m = duc.NumVectors();
  for (int k = 0; k < m; k++) {
    for (int i = 0; i < n; i++) {
      diag[k][i] += volc[0][i] * duc[k][i] / dT;
    } 
  }
}


/* ******************************************************************
* Linearized update methods with storage terms for component "name".
* Op  += ss * vol / dt
* RHS += s0 * vol * u0 / dt
****************************************************************** */
void Accumulation::AddAccumulationDelta(
    const CompositeVector& u0,
    const CompositeVector& s0, const CompositeVector& ss,
    double dT, const std::string& name)
{
  Teuchos::RCP<Op> op = FindOp_(name);
  Epetra_MultiVector& diag = *op->diag;

  CompositeVector vol(ss);
  CalculateEntitylVolume_(vol, name);

  const Epetra_MultiVector& u0c = *u0.ViewComponent(name);
  const Epetra_MultiVector& s0c = *s0.ViewComponent(name);
  const Epetra_MultiVector& ssc = *ss.ViewComponent(name);

  Epetra_MultiVector& volc = *vol.ViewComponent(name); 
  Epetra_MultiVector& rhs = *global_operator()->rhs()->ViewComponent(name);

  int n = u0c.MyLength();
  int m = u0c.NumVectors();
  for (int k = 0; k < m; ++k) {
    for (int i = 0; i < n; i++) {
      double factor = volc[0][i] / dT;
      diag[k][i] += factor * ssc[k][i];
      rhs[k][i] += factor * s0c[k][i] * u0c[k][i];
    }
  }
}


/* ******************************************************************
* Linearized update methods with storage terms for component "name".
* Op  += vol / dt
* RHS += vol * u0 / dt
****************************************************************** */
void Accumulation::AddAccumulationDelta(
    const CompositeVector& u0,
    double dT, const std::string& name)
{
  Teuchos::RCP<Op> op = FindOp_(name);
  Epetra_MultiVector& diag = *op->diag;

  CompositeVector vol(u0);
  CalculateEntitylVolume_(vol, name);

  const Epetra_MultiVector& u0c = *u0.ViewComponent(name);
  Epetra_MultiVector& volc = *vol.ViewComponent(name); 
  Epetra_MultiVector& rhs = *global_operator()->rhs()->ViewComponent(name);

  int n = u0c.MyLength();
  int m = u0c.NumVectors();
  for (int k = 0; k < m; ++k) {
    for (int i = 0; i < n; i++) {
      double factor = volc[0][i] / dT;
      diag[k][i] += factor;
      rhs[k][i] += factor * u0c[k][i];
    }
  }
}


/* ******************************************************************
* Linearized update methods with storage terms for component "name".
* Op  += ss
* RHS += ss * u0
****************************************************************** */
void Accumulation::AddAccumulationDeltaNoVolume(
    const CompositeVector& u0, const CompositeVector& ss, const std::string& name)
{
  if (!ss.HasComponent(name)) ASSERT(false);

  Teuchos::RCP<Op> op = FindOp_(name);
  Epetra_MultiVector& diag = *op->diag;

  const Epetra_MultiVector& u0c = *u0.ViewComponent(name);
  const Epetra_MultiVector& ssc = *ss.ViewComponent(name);

  Epetra_MultiVector& rhs = *global_operator()->rhs()->ViewComponent(name);

  int n = u0c.MyLength();
  int m = u0c.NumVectors();
  for (int k = 0; k < m; ++k) {
    for (int i = 0; i < n; i++) {
      diag[k][i] += ssc[k][i];
      rhs[k][i] += ssc[k][i] * u0c[k][i];
    }
  }
}


/* ******************************************************************
* Calculate entity volume for component "name" of vector ss.
****************************************************************** */
void Accumulation::CalculateEntitylVolume_(
    CompositeVector& volume, const std::string& name)
{
  AmanziMesh::Entity_ID_List nodes, edges;

  if (name == "cell" && volume.HasComponent("cell")) {
    Epetra_MultiVector& vol = *volume.ViewComponent(name); 

    for (int c = 0; c != ncells_owned; ++c) {
      vol[0][c] = mesh_->cell_volume(c); 
    }

  } else if (name == "face" && volume.HasComponent("face")) {
    // Missing code.
    ASSERT(false);

  } else if (name == "edge" && volume.HasComponent("edge")) {
    Epetra_MultiVector& vol = *volume.ViewComponent(name, true); 
    vol.PutScalar(0.0);

    for (int c = 0; c != ncells_owned; ++c) {
      mesh_->cell_get_edges(c, &edges);
      int nedges = edges.size();

      for (int i = 0; i < nedges; i++) {
        vol[0][edges[i]] += mesh_->cell_volume(c) / nedges; 
      }
    }
    volume.GatherGhostedToMaster(name);

  } else if (name == "node" && volume.HasComponent("node")) {
    Epetra_MultiVector& vol = *volume.ViewComponent(name, true); 
    vol.PutScalar(0.0);

    for (int c = 0; c != ncells_owned; ++c) {
      mesh_->cell_get_nodes(c, &nodes);
      int nnodes = nodes.size();

      for (int i = 0; i < nnodes; i++) {
        vol[0][nodes[i]] += mesh_->cell_volume(c) / nnodes; 
      }
    }
    volume.GatherGhostedToMaster(name);

  } else {
    ASSERT(false);
  }
}


/* ******************************************************************
* Note: When complex schema is used to create a set of local ops, the
* the local local_op_ is not well defined.
****************************************************************** */
void Accumulation::InitAccumulation_(const Schema& schema, bool surf)
{
  Errors::Message msg;

  if (global_op_ == Teuchos::null) {
    // constructor was given a mesh 
    Teuchos::ParameterList plist;

    global_op_schema_ = schema;
    local_op_schema_ = schema;

    for (auto it = schema.begin(); it != schema.end(); ++it) {
      Teuchos::RCP<Op> op;
      Teuchos::RCP<CompositeVectorSpace> cvs = Teuchos::rcp(new CompositeVectorSpace());
      cvs->SetMesh(mesh_)->AddComponent(schema.KindToString(it->kind), it->kind, it->num);

      if (it->kind == AmanziMesh::CELL) {
        int old_schema = OPERATOR_SCHEMA_BASE_CELL | OPERATOR_SCHEMA_DOFS_CELL;
        global_op_ = Teuchos::rcp(new Operator_Cell(cvs, plist, old_schema));
        std::string name("CELL_CELL");
        if (surf) {
          op = Teuchos::rcp(new Op_SurfaceCell_SurfaceCell(name, mesh_));
        } else {
          op = Teuchos::rcp(new Op_Cell_Cell(name, mesh_));
        }

      /*
      } else if (it->kind == AmanziMesh::FACE) {
        global_op_ = Teuchos::rcp(new Operator_Face(cvs, plist));
        std::string name("FACE_FACE");
        op = Teuchos::rcp(new Op_Face_Face(name, mesh_));
      */

      } else if (it->kind == AmanziMesh::EDGE) {
        global_op_ = Teuchos::rcp(new Operator_Edge(cvs, plist));
        std::string name("EDGE_EDGE");
        op = Teuchos::rcp(new Op_Edge_Edge(name, mesh_));

      } else if (it->kind == AmanziMesh::NODE) {
        global_op_ = Teuchos::rcp(new Operator_Node(cvs, plist));
        std::string name("NODE_NODE");
        op = Teuchos::rcp(new Op_Node_Node(name, mesh_));

      } else {
        msg << "Accumulation operator: Unknown kind \"" << schema.KindToString(it->kind) << "\".\n";
        Exceptions::amanzi_throw(msg);
      }

      global_op_->OpPushBack(op);
      local_ops_.push_back(op);
    }

  } else {
    // constructor was given an Operator
    global_op_schema_ = global_op_->schema_row();
    mesh_ = global_op_->DomainMap().Mesh();

    for (auto it = schema.begin(); it != schema.end(); ++it) {
      int old_schema;
      Teuchos::RCP<Op> op;

      if (it->kind == AmanziMesh::CELL) {
        old_schema = OPERATOR_SCHEMA_BASE_CELL | OPERATOR_SCHEMA_DOFS_CELL;
        std::string name("CELL_CELL");
        if (surf) {
          op = Teuchos::rcp(new Op_SurfaceCell_SurfaceCell(name, mesh_));
        } else {
          op = Teuchos::rcp(new Op_Cell_Cell(name, mesh_));
        }

      /*
      } else if (it->kind == AmanziMesh::FACE) {
        old_schema = OPERATOR_SCHEMA_BASE_FACE | OPERATOR_SCHEMA_DOFS_FACE;
        std::string name("FACE_FACE");
        op = Teuchos::rcp(new Op_Face_Face(name, mesh_));
      */

      } else if (it->kind == AmanziMesh::EDGE) {
        old_schema = OPERATOR_SCHEMA_BASE_EDGE | OPERATOR_SCHEMA_DOFS_EDGE;
        std::string name("EDGE_EDGE");
        op = Teuchos::rcp(new Op_Edge_Edge(name, mesh_));

      } else if (it->kind == AmanziMesh::NODE) {
        old_schema = OPERATOR_SCHEMA_BASE_NODE | OPERATOR_SCHEMA_DOFS_NODE;
        std::string name("NODE_NODE");
        op = Teuchos::rcp(new Op_Node_Node(name, mesh_));

      } else {
        msg << "Accumulation operator: Unknown kind \"" << schema.KindToString(it->kind) << "\".\n";
        Exceptions::amanzi_throw(msg);
      }

      // register the accumulation Op
      local_op_schema_.Init(old_schema);
      global_op_->OpPushBack(op);
      local_ops_.push_back(op);
    }
  }

  // mesh info
  ncells_owned = mesh_->num_entities(AmanziMesh::CELL, AmanziMesh::OWNED);
  nfaces_owned = mesh_->num_entities(AmanziMesh::FACE, AmanziMesh::OWNED);
  nnodes_owned = mesh_->num_entities(AmanziMesh::NODE, AmanziMesh::OWNED);
}


/* ******************************************************************
* Apply boundary conditions to 
****************************************************************** */
void Accumulation::ApplyBCs(const Teuchos::RCP<BCs>& bc)
{
  const std::vector<int>& bc_model = bc->bc_model();

  for (auto it = local_ops_.begin(); it != local_ops_.end(); ++it) {
    const Schema& schema = (*it)->schema_row();
    if (schema.base() == bc->kind()) {
      Epetra_MultiVector& diag = *(*it)->diag;

      for (int i = 0; i < diag.MyLength(); i++) {
        if (bc_model[i] == OPERATOR_BC_DIRICHLET) {
          diag[0][i] = 0.0;
        }
      }
    }
  }
}


/* ******************************************************************
* Apply boundary conditions to 
****************************************************************** */
Teuchos::RCP<Op> Accumulation::FindOp_(const std::string& name) const
{
  for (auto it = local_ops_.begin(); it != local_ops_.end(); ++it) {
    const Schema& schema = (*it)->schema_row();
    if (schema.KindToString(schema.base()) == name) 
      return *it;
  }
  return Teuchos::null;
}

}  // namespace Operators
}  // namespace Amanzi



