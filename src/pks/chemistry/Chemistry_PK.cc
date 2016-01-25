/*
  Chemistry PK

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Base class for chemical process kernels.
*/
 
#include "Chemistry_PK.hh"

namespace Amanzi {
namespace AmanziChemistry {

/* ******************************************************************
* Default constructor that initializes all pointers to NULL
****************************************************************** */
Chemistry_PK::Chemistry_PK() :
    passwd_("state"),
    number_minerals_(0),
    number_ion_exchange_sites_(0),
    number_sorption_sites_(0),
    using_sorption_(false),
    using_sorption_isotherms_(false) {};


/* ******************************************************************
* Register fields and evaluators with the State
******************************************************************* */
void Chemistry_PK::Setup()
{
  // Require data from flow
  if (!S_->HasField("porosity")) {
    S_->RequireField("porosity", passwd_)->SetMesh(mesh_)->SetGhosted(false)
      ->SetComponent("cell", AmanziMesh::CELL, 1);
  }

  if (!S_->HasField("saturation_liquid")) {
    S_->RequireField("saturation_liquid", passwd_)->SetMesh(mesh_)->SetGhosted(false)
      ->SetComponent("cell", AmanziMesh::CELL, 1);
  }
  
  if (!S_->HasField("fluid_density")) {
    S_->RequireScalar("fluid_density", passwd_);
  }

  // require transport fields
  std::vector<std::string>::const_iterator it;
  if (!S_->HasField("total_component_concentration")) {    
    // set the names for vis
    std::vector<std::vector<std::string> > conc_names_cv(1);
    for (it = comp_names_.begin(); it != comp_names_.end(); ++it) {
      conc_names_cv[0].push_back(*it + std::string(" conc"));
    }
    S_->RequireField("total_component_concentration", passwd_, conc_names_cv)
      ->SetMesh(mesh_)->SetGhosted(true)
      ->SetComponent("cell", AmanziMesh::CELL, number_aqueous_components_);
  }

  // require minerals
  if (number_minerals_ > 0) {
    // -- set the names for vis
    std::vector<std::vector<std::string> > vf_names_cv(1);
    std::vector<std::vector<std::string> > ssa_names_cv(1);

    for (it = mineral_names_.begin(); it != mineral_names_.end(); ++it) {
      vf_names_cv[0].push_back(*it + std::string(" vol frac"));
      ssa_names_cv[0].push_back(*it + std::string(" spec surf area"));
    }

    // -- register two fields
    S_->RequireField("mineral_volume_fractions", passwd_, vf_names_cv)
      ->SetMesh(mesh_)->SetGhosted(false)
      ->SetComponent("cell", AmanziMesh::CELL, number_minerals_);

    S_->RequireField("mineral_specific_surface_area", passwd_, ssa_names_cv)
      ->SetMesh(mesh_)->SetGhosted(false)
      ->SetComponent("cell", AmanziMesh::CELL, number_minerals_);
  }

  // require sorption sites
  if (number_sorption_sites_ > 0) {
    // -- set the names for vis
    std::vector<std::vector<std::string> > ss_names_cv(1);
    std::vector<std::vector<std::string> > scfsc_names_cv(1);
    for (it = sorption_site_names_.begin(); it != sorption_site_names_.end(); ++it) {
      ss_names_cv[0].push_back(*it + std::string(" sorption site"));
      scfsc_names_cv[0].push_back(*it + std::string(" surface complex free site conc"));
    }

    // -- register two fields
    S_->RequireField("sorption_sites", passwd_, ss_names_cv)
      ->SetMesh(mesh_)->SetGhosted(false)
      ->SetComponent("cell", AmanziMesh::CELL, number_sorption_sites_);
    S_->RequireField("surface_complex_free_site_conc", passwd_, scfsc_names_cv)
      ->SetMesh(mesh_)->SetGhosted(false)
      ->SetComponent("cell", AmanziMesh::CELL, number_sorption_sites_);
  }

  if (using_sorption_) {
    S_->RequireField("total_sorbed", passwd_)
      ->SetMesh(mesh_)->SetGhosted(false)
      ->SetComponent("cell", AmanziMesh::CELL, number_aqueous_components_);

    if (using_sorption_isotherms_) {
      S_->RequireField("isotherm_kd", passwd_)
        ->SetMesh(mesh_)->SetGhosted(false)
        ->SetComponent("cell", AmanziMesh::CELL, number_aqueous_components_);

      S_->RequireField("isotherm_freundlich_n", passwd_)
        ->SetMesh(mesh_)->SetGhosted(false)
        ->SetComponent("cell", AmanziMesh::CELL, number_aqueous_components_);

      S_->RequireField("isotherm_langmuir_b", passwd_)
        ->SetMesh(mesh_)->SetGhosted(false)
        ->SetComponent("cell", AmanziMesh::CELL, number_aqueous_components_);
    }
  }

  // ion
  if (number_aqueous_components_ > 0) {
    std::vector<std::vector<std::string> > species_names_cv(1);
    for (it = comp_names_.begin(); it != comp_names_.end(); ++it) {
      species_names_cv[0].push_back(*it);
    }

    S_->RequireField("free_ion_species", passwd_, species_names_cv)
      ->SetMesh(mesh_)->SetGhosted(false)
      ->SetComponent("cell", AmanziMesh::CELL, number_aqueous_components_);

    S_->RequireField("primary_activity_coeff", passwd_, species_names_cv)
      ->SetMesh(mesh_)->SetGhosted(false)
      ->SetComponent("cell", AmanziMesh::CELL, number_aqueous_components_);
  }

  if (number_ion_exchange_sites_ > 0) {
    S_->RequireField("ion_exchange_sites", passwd_)
      ->SetMesh(mesh_)->SetGhosted(false)
      ->SetComponent("cell", AmanziMesh::CELL, number_ion_exchange_sites_);

    S_->RequireField("ion_exchange_ref_cation_conc", passwd_)
      ->SetMesh(mesh_)->SetGhosted(false)
      ->SetComponent("cell", AmanziMesh::CELL, number_ion_exchange_sites_);
  }
}


/* ******************************************************************
* Most things are initialized through State, but State can only manage that
* if they are always initialized.  If sane defaults are available, or they
* can be derived from other initialized quantities, they are initialized
* here, where we can manage that logic.
******************************************************************* */
void Chemistry_PK::Initialize()
{
  // Aqueous species 
  if (number_aqueous_components_ > 0) {
    if (!S_->GetField("total_component_concentration", passwd_)->initialized()) {
      InitializeField_("total_component_concentration", 0.0);
    }
    InitializeField_("free_ion_species", 0.0);
    InitializeField_("primary_activity_coeff", 1.0);

    // Sorption sites: all will have a site density, but we can default to zero
    if (using_sorption_) {
      InitializeField_("total_sorbed", 0.0);
    }

    // Sorption isotherms: Kd required, Langmuir and Freundlich optional
    if (using_sorption_isotherms_) {
      InitializeField_("isotherm_kd", -1.0);
      InitializeField_("isotherm_freundlich_n", 1.0);
      InitializeField_("isotherm_langmuir_b", 1.0);
    }
  }

  // Minerals: vol frac and surface areas
  if (number_minerals_ > 0) {
    InitializeField_("mineral_volume_fractions", 0.0);
    InitializeField_("mineral_specific_surface_area", 1.0);
  }

  // Ion exchange sites: default to 1
  if (number_ion_exchange_sites_ > 0) {
    InitializeField_("ion_exchange_sites", 1.0);
    InitializeField_("ion_exchange_ref_cation_conc", 1.0);
  }

  if (number_sorption_sites_ > 0) {
    InitializeField_("sorption_sites", 1.0);
    InitializeField_("surface_complex_free_site_conc", 1.0);
  }
}


/* ******************************************************************
* Process names of materials 
******************************************************************* */
void Chemistry_PK::InitializeField_(std::string fieldname, double default_val)
{
  Teuchos::OSTab tab = vo_->getOSTab();

  if (S_->HasField(fieldname)) {
    if (!S_->GetField(fieldname)->initialized()) {
      S_->GetFieldData(fieldname, passwd_)->PutScalar(default_val);
      S_->GetField(fieldname, passwd_)->set_initialized();
      if (vo_->getVerbLevel() >= Teuchos::VERB_MEDIUM)
         *vo_->os() << "initilized " << fieldname << " to value " << default_val << std::endl;  
    }
  }
}


/* ******************************************************************
* Process names of materials 
******************************************************************* */
void Chemistry_PK::InitializeMinerals(Teuchos::RCP<Teuchos::ParameterList> plist)
{
  mineral_names_.clear();
  if (plist->isParameter("Minerals")) {
    mineral_names_ = plist->get<Teuchos::Array<std::string> >("Minerals").toVector();
  }

  number_minerals_ = mineral_names_.size();
}


/* ******************************************************************
* Process names of sorption sites
* NOTE: Do we need to worry about sorption sites?
******************************************************************* */
void Chemistry_PK::InitializeSorptionSites(Teuchos::RCP<Teuchos::ParameterList> plist,
                                           Teuchos::RCP<Teuchos::ParameterList> state_list)
{
  sorption_site_names_.clear();
  if (plist->isParameter("Sorption Sites")) {
    sorption_site_names_ = plist->get<Teuchos::Array<std::string> >("Sorption Sites").toVector();
  } 
  
  number_sorption_sites_ = sorption_site_names_.size();
  using_sorption_ = (number_sorption_sites_ > 0);

  // check if there is an initial condition for ion_exchange_sites
  number_ion_exchange_sites_ = 0;
  using_sorption_isotherms_ = false;

  if (state_list->sublist("initial conditions").isSublist("ion_exchange_sites")) {
    // there is currently only at most one site...
    using_sorption_ = true;
    number_ion_exchange_sites_ = 1;
  }

  if (state_list->sublist("initial conditions").isSublist("isotherm_kd")) {
    using_sorption_ = true;
    using_sorption_isotherms_ = true;
  }

  if (state_list->sublist("initial conditions").isSublist("sorption_sites")) {
    using_sorption_ = true;
  }

  // in the old version, this was only in the Block sublist... may need work?
  if (plist->isParameter("Cation Exchange Capacity")) {
    using_sorption_ = true;
    number_ion_exchange_sites_ = 1;
  }
} 

}  // namespace AmanziChemistry
}  // namespace Amanzi
