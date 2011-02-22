#ifndef _MESH_HH_
#define _MESH_HH_

#include <iosfwd>
#include "Mesh_maps_base.hh"
#include "Entity_map.hh"
#include "Data_structures.hh"

#include <Teuchos_RCP.hpp>
#include <Epetra_MpiComm.h>
#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/fem/FieldTraits.hpp>

#include <map>
#include <memory>

namespace STK_mesh
{

class Mesh
{

private:

    Epetra_MpiComm communicator_;
    Teuchos::RCP<Entity_map> entity_map_;
    
    int space_dimension_;
    bool consistent_;
    
    std::auto_ptr<stk::mesh::MetaData> meta_data_;
    std::auto_ptr<stk::mesh::BulkData> bulk_data_;
    
    Vector_field_type &coordinate_field_;
    Id_field_type *face_owner_;
    
    stk::mesh::Selector selector_ (Element_Category category) const;

    Id_map set_to_part_;
    
    const stk::mesh::Part& get_part_from_set_id_ (unsigned int set_id);

    void get_entities_ (const stk::mesh::Selector& selector, stk::mesh::EntityRank rank, Entity_vector&) const;

    void remove_bad_ghosts_(Entity_vector& entities) const;

    // Internal Validators
    bool element_type_ok_ () const;
    bool dimension_ok_ () const;
    
    // Disable copy and assignment.
    Mesh (const Mesh& rhs);
    Mesh& operator=(const Mesh& rhs);
    
public:
    
    
    // Structors
    // ---------
    
    Mesh (int space_dimension, 
          const Epetra_MpiComm& communicator, 
          Entity_map* entity_map, 
          stk::mesh::MetaData *meta_data, 
          stk::mesh::BulkData *bulk_data,
          const Id_map &set_to_part,
          Vector_field_type& coordinate_field);
    
    virtual ~Mesh () { }
    
    
    // Accessors
    // ---------
    
    int space_dimension () const { return space_dimension_;  }
    
    const stk::mesh::MetaData& meta_data    () const { return *meta_data_; }
    const stk::mesh::BulkData& build_data   () const { return *bulk_data_; }
    const Entity_map&          entity_map   () const { return *entity_map_; }
    const Epetra_MpiComm&      communicator () const { return communicator_; }
    bool                       consistent   () const { return consistent_; }
    unsigned int               rank_id      () const { return communicator_.MyPID (); }
    
    unsigned int count_entities (stk::mesh::EntityRank rank,  Element_Category category) const;
    unsigned int count_entities (const stk::mesh::Part& part, Element_Category category) const;
    
    void get_entities (stk::mesh::EntityRank rank,  Element_Category category, Entity_vector& entities) const;
    void get_entities (const stk::mesh::Part& part, Element_Category category, Entity_vector& entities) const;
    
    void element_to_faces (stk::mesh::EntityId element, Entity_Ids& ids) const;
    void element_to_face_dirs (stk::mesh::EntityId element, std::vector<int>& ids) const;
    void element_to_nodes (stk::mesh::EntityId element, Entity_Ids& ids) const;
    void face_to_nodes    (stk::mesh::EntityId element, Entity_Ids& ids) const;
    void face_to_elements (stk::mesh::EntityId element, Entity_Ids& ids) const;
    
    double const * coordinates (stk::mesh::EntityId node) const;
    double const * coordinates (stk::mesh::Entity* node)  const;
    
    stk::mesh::Entity* id_to_entity (stk::mesh::EntityRank rank, 
                                     stk::mesh::EntityId id,
                                     Element_Category category) const;


    // Sets
    // ----

    unsigned int num_sets () const { return set_to_part_.size (); }
    unsigned int num_sets (stk::mesh::EntityRank rank) const;

    Id_map::const_iterator sets_begin () const { return set_to_part_.begin (); }
    Id_map::const_iterator sets_end   () const { return set_to_part_.end ();   }
    
    bool valid_id (unsigned int id, stk::mesh::EntityRank rank) const;

    stk::mesh::Part* get_set (unsigned int set_id, stk::mesh::EntityRank rank);
    stk::mesh::Part* get_set (const char* name,    stk::mesh::EntityRank rank);

    void get_sets (stk::mesh::EntityRank rank, stk::mesh::PartVector& sets) const;
    void get_set_ids (stk::mesh::EntityRank rank, std::vector<unsigned int>&) const;


    
    
    // Static information
    // ------------------

    static stk::mesh::EntityRank get_element_type (int space_dimension);
    static stk::mesh::EntityRank get_face_type    (int space_dimension);


    // Validators
    // ----------

    static bool valid_dimension (int space_dimension);
    static bool valid_rank (stk::mesh::EntityRank);

    /// redistribute cell ownership according to the specified map
    void redistribute(const Epetra_Map& cellmap);

    void summary(std::ostream& os) const;
    
}; // close class Mesh

} // close namespace STK_mesh

#endif
