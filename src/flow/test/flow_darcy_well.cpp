/*
This is the flow component of the Amanzi code. 

Copyright 2010-2012 held jointly by LANS/LANL, LBNL, and PNNL. 
Amanzi is released under the three-clause BSD License. 
The terms of use and "as is" disclaimer for this license are 
provided Reconstruction.cppin the top-level COPYRIGHT file.

Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "UnitTest++.h"

#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_ParameterXMLFileReader.hpp"

#include "MeshFactory.hh"
#include "MeshAudit.hh"
#include "gmv_mesh.hh"

#include "State.hpp"
#include "Flow_State.hpp"
#include "Darcy_PK.hpp"


/* **************************************************************** */
TEST(FLOW_2D_DARCY_WELL) {
  using namespace Teuchos;
  using namespace Amanzi;
  using namespace Amanzi::AmanziMesh;
  using namespace Amanzi::AmanziGeometry;
  using namespace Amanzi::AmanziFlow;

  Epetra_MpiComm comm(MPI_COMM_WORLD);
  int MyPID = comm.MyPID();

  if (MyPID == 0) cout << "Test: 2D specific storage Darcy, homogeneous medium" << endl;

  /* read parameter list */
  ParameterList parameter_list;
  string xmlFileName = "test/flow_darcy_well.xml";

  ParameterXMLFileReader xmlreader(xmlFileName);
  parameter_list = xmlreader.getParameters();

  // create an MSTK mesh framework
  ParameterList region_list = parameter_list.get<Teuchos::ParameterList>("Regions");
  GeometricModelPtr gm = new GeometricModel(2, region_list, &comm);

  FrameworkPreference pref;
  pref.clear();
  pref.push_back(MSTK);

  MeshFactory meshfactory(&comm);
  meshfactory.preference(pref);

  RCP<Mesh> mesh = meshfactory(-10.0, -5.0, 10.0, 0.0, 200, 50, gm);

  // create and populate flow state
  Teuchos::RCP<Flow_State> FS = Teuchos::rcp(new Flow_State(mesh));
  FS->set_permeability(0.1, 2.0, "Computational domain");
  FS->set_porosity(0.2);
  FS->set_specific_storage(1e-1);
  FS->set_fluid_viscosity(1.0);
  FS->set_fluid_density(1.0);
  FS->set_gravity(-1.0);

  // create Richards process kernel
  Darcy_PK* DPK = new Darcy_PK(parameter_list, FS);
  DPK->InitPK();
  DPK->InitSteadyState(0.0, 1e-8);

  // create the initial pressure function
  Epetra_Vector& p = FS->ref_pressure();

  for (int c = 0; c < p.MyLength(); c++) {
    const Point& xc = mesh->cell_centroid(c);
    p[c] = xc[1] * (xc[1] + 2.0);
  }

  // transient solution
  double dT = 0.5;
  for (int n = 0; n < 10; n++) {
    DPK->Advance(dT);
    DPK->CommitState(FS);

    if (MyPID == 0) {
      GMV::open_data_file(*mesh, (std::string)"flow.gmv");
      GMV::start_data();
      GMV::write_cell_data(FS->ref_pressure(), 0, "pressure");
      GMV::close_data_file();
    }
  }

  delete DPK;
}
