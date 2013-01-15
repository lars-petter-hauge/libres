/*
   Copyright (C) 2012  Statoil ASA, Norway. 
    
   The file 'ert_workflow_list.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include <hash.h>
#include <stringlist.h>
#include <util.h>
#include <subst_list.h>

#include <config.h>
#include <config_schema_item.h>

#include <workflow.h>
#include <workflow_job.h>
#include <workflow_joblist.h>

#include <ert_workflow_list.h>
#include <config_keys.h>


struct ert_workflow_list_struct {
  stringlist_type       * path_list;
  hash_type             * workflows;
  workflow_joblist_type * joblist;
  const subst_list_type * context;
};



ert_workflow_list_type * ert_workflow_list_alloc(const subst_list_type * context) {
  ert_workflow_list_type * workflow_list = util_malloc( sizeof * workflow_list );
  workflow_list->path_list = stringlist_alloc_new();
  workflow_list->workflows = hash_alloc();
  workflow_list->joblist   = workflow_joblist_alloc();
  workflow_list->context   = context;
  return workflow_list;
}



void ert_workflow_list_free( ert_workflow_list_type * workflow_list ) {
  hash_free( workflow_list->workflows );
  stringlist_free( workflow_list->path_list );
  workflow_joblist_free( workflow_list->joblist );
  free( workflow_list );
}



workflow_type * ert_workflow_list_add_workflow( ert_workflow_list_type * workflow_list , const char * workflow_file , const char * workflow_name) {
  if (util_file_exists( workflow_file )) {
    workflow_type * workflow = workflow_alloc( workflow_file , workflow_list->joblist );
    if (workflow_name == NULL) 
      hash_insert_hash_owned_ref( workflow_list->workflows , workflow_name , workflow , workflow_free__);    
    else {
      char * name;
      util_alloc_file_components( workflow_file , NULL , &name , NULL );
      hash_insert_hash_owned_ref( workflow_list->workflows , name , workflow , workflow_free__);    
      free( name );
    }
    return workflow;
  } else
    return NULL;
}



void ert_workflow_list_add_alias( ert_workflow_list_type * workflow_list , const char * real_name , const char * alias) {
  workflow_type * workflow = ert_workflow_list_get_workflow( workflow_list , real_name );
  hash_insert_ref( workflow_list->workflows , alias , workflow );
}


void ert_workflow_list_add_job( ert_workflow_list_type * workflow_list , const char * job_name , const char * config_file ) {
  char * name = (char *) job_name;

  if (job_name == NULL) 
    util_alloc_file_components( config_file , NULL , &name , NULL );
  
  if (!workflow_joblist_add_job_from_file( workflow_list->joblist , name , config_file )) 
    fprintf(stderr,"** Warning: failed to add workflow job:%s from:%s \n",job_name , config_file );

  if (job_name == NULL) 
    free(name);
}



void ert_workflow_list_add_jobs_in_directory( ert_workflow_list_type * workflow_list , const char * path ) {
  DIR * dirH = opendir( path );
  while (true) {
    struct dirent * entry = readdir( dirH );
    if (entry != NULL) {
      if ((strcmp(entry->d_name , ".") != 0) && (strcmp(entry->d_name , "..") != 0)) {
        char * full_path = util_alloc_filename( path , entry->d_name , NULL );

        if (util_is_file( full_path ))
          ert_workflow_list_add_job( workflow_list , entry->d_name , full_path );
        
        free( full_path );
      }
    } else 
      break;
  }
  closedir( dirH );
}


void ert_workflow_list_init( ert_workflow_list_type * workflow_list , config_type * config ) {
  /* Adding jobs */
  {
    const config_content_item_type * jobpath_item = config_get_content_item( config , WORKFLOW_JOB_DIRECTORY_KEY);
    if (jobpath_item != NULL) {
      for (int i=0; i < config_content_item_get_size( jobpath_item ); i++) {
        config_content_node_type * path_node = config_content_item_iget_node( jobpath_item , i );
        for (int j=0; j < config_content_node_get_size( path_node ); j++) 
          ert_workflow_list_add_jobs_in_directory( workflow_list , config_content_node_iget_as_path( path_node , j ));
      }
    }
  }
  
  {
    const config_content_item_type * job_item = config_get_content_item( config , LOAD_WORKFLOW_JOB_KEY);
    if (job_item != NULL) {
      for (int i=0; i < config_content_item_get_size( job_item ); i++) {
        config_content_node_type * job_node = config_content_item_iget_node( job_item , i );
        const char * config_file = config_content_node_iget_as_path( job_node , 0 );
        const char * job_name = config_content_node_safe_iget( job_node , 1 );
        ert_workflow_list_add_job( workflow_list , job_name , config_file);
      }
    }
  }
  

  /* Adding workflows */
  {
    const config_content_item_type * workflow_item = config_get_content_item( config , LOAD_WORKFLOW_KEY);

    if (workflow_item != NULL) {
      for (int i=0; i < config_content_item_get_size( workflow_item ); i++) {
        config_content_node_type * workflow_node = config_content_item_iget_node( workflow_item , i );
        const char * workflow_file = config_content_node_iget_as_path( workflow_node , 0 );
        const char * workflow_name = config_content_node_safe_iget( workflow_node , 1 );
        
        ert_workflow_list_add_workflow( workflow_list , workflow_file , workflow_name );
      }
    }
  }
}


void ert_workflow_list_update_config( config_type * config ) {
  config_schema_item_type * item = config_add_schema_item( config , WORKFLOW_JOB_DIRECTORY_KEY , false  );
  config_schema_item_set_argc_minmax(item , 1 , 1 , 1 , (const config_item_types [1]) { CONFIG_EXISTING_PATH } );

  item = config_add_schema_item( config , LOAD_WORKFLOW_KEY , false  );
  config_schema_item_set_argc_minmax(item , 1 , 2 , 2 ,  (const config_item_types [2]) { CONFIG_EXISTING_PATH , CONFIG_STRING });
  
  item = config_add_schema_item( config , LOAD_WORKFLOW_JOB_KEY , false  );
  config_schema_item_set_argc_minmax(item , 1 , 2 , 2 ,  (const config_item_types [2]) { CONFIG_EXISTING_PATH , CONFIG_STRING});
}



workflow_type *  ert_workflow_list_get_workflow(ert_workflow_list_type * workflow_list , const char * workflow_name ) {
  return hash_get( workflow_list->workflows , workflow_name );
}

bool  ert_workflow_list_has_workflow(ert_workflow_list_type * workflow_list , const char * workflow_name ) {
  return hash_has_key( workflow_list->workflows , workflow_name );
}


bool ert_workflow_list_run_workflow__(ert_workflow_list_type * workflow_list  , workflow_type * workflow, void * self ) {
  return workflow_run( workflow , self , workflow_list->context );
}

bool ert_workflow_list_run_workflow(ert_workflow_list_type * workflow_list  , const char * workflow_name , void * self) {
  workflow_type * workflow = ert_workflow_list_get_workflow( workflow_list , workflow_name );
  return ert_workflow_list_run_workflow__( workflow_list , workflow , self );
}


/*****************************************************************/

stringlist_type * ert_workflow_list_alloc_namelist( ert_workflow_list_type * workflow_list ) {
  return hash_alloc_stringlist( workflow_list->workflows );
}

