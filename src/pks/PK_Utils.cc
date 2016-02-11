/*
  This is the process kernel component of the Amanzi code. 

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Miscalleneous collection of simple non-member functions.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include "CompositeVector.hh"
#include "PK_Utils.hh"

namespace Amanzi {

/* ******************************************************************
* Averages permeability tensor in horizontal direction.
****************************************************************** */
void PKUtils_CalculatePermeabilityFactorInWell(
    const Teuchos::Ptr<State>& S, Teuchos::RCP<Epetra_Vector>& Kxy)
{
  const CompositeVector& cv = *S->GetFieldData("permeability");
  cv.ScatterMasterToGhosted("cell");
  const Epetra_MultiVector& perm = *cv.ViewComponent("cell", true);
 
  int ncells_wghost = S->GetMesh()->num_entities(AmanziMesh::CELL, AmanziMesh::USED);
  int dim = S->GetMesh()->space_dimension();

  Kxy = Teuchos::rcp(new Epetra_Vector(S->GetMesh()->cell_map(true)));

  for (int c = 0; c < ncells_wghost; c++) {
    (*Kxy)[c] = 0.0;
    int idim = std::max(1, dim - 1);
    for (int i = 0; i < idim; i++) (*Kxy)[c] += perm[i][c];
    (*Kxy)[c] /= idim;
  }
}

}  // namespace Amanzi
