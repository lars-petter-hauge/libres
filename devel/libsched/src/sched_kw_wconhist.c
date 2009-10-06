#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <sched_kw_wconhist.h>
#include <sched_util.h>
#include <vector.h>
#include <hash.h>
#include <stringlist.h>

/*
  Define the maximum number of keywords in a WCONHIST record.
  Note that this includes wet gas rate, which is only supported
  by ECL 300.
*/

#define WCONHIST_NUM_KW       11
#define SCHED_KW_WCONHIST_ID  771054  /* Very random intgere for checking type-cast. */
#define DEFAULT_STATUS OPEN


typedef enum {OPEN, STOP, SHUT} st_flag_type;
#define ST_OPEN_STRING "OPEN"
#define ST_STOP_STRING "STOP"
#define ST_SHUT_STRING "SHUT"

typedef enum {ORAT, WRAT, GRAT, LRAT, RESV} cm_flag_type;
#define CM_ORAT_STRING "ORAT"
#define CM_WRAT_STRING "WRAT"
#define CM_GRAT_STRING "GRAT"
#define CM_LRAT_STRING "LRAT"
#define CM_RESV_STRING "RESV"


typedef struct wconhist_well_struct wconhist_well_type;



struct wconhist_well_struct{
  /* 
    def: Read as defaulted, not defined!
  */
  bool          def[WCONHIST_NUM_KW];

  char        * name;
  st_flag_type  status;
  cm_flag_type  cmode;
  double        orat;
  double        wrat;
  double        grat;
  int           vfptable;
  double        alift;
  double        thp;
  double        bhp;
  double        wgrat;
};



struct sched_kw_wconhist_struct{
  UTIL_TYPE_ID_DECLARATION;
  vector_type * wells;
};





static char * get_st_string(st_flag_type status)
{
  switch(status)
  {
    case(OPEN):
     return ST_OPEN_STRING; 
    case(STOP):
      return ST_STOP_STRING;
    case(SHUT):
      return ST_SHUT_STRING;
    default:
      return SCHED_KW_DEFAULT_ITEM;
  }
}



static char * get_cm_string(cm_flag_type cmode)
{
  switch(cmode)
  {
    case(ORAT):
      return CM_ORAT_STRING;
    case(WRAT):
      return CM_WRAT_STRING;
    case(GRAT):
      return CM_GRAT_STRING;
    case(LRAT):
      return CM_LRAT_STRING;
    case(RESV):
      return CM_RESV_STRING;
    default:
      return SCHED_KW_DEFAULT_ITEM;
  }
}



static st_flag_type get_st_flag_from_string(const char * st_string)
{
  if (strcmp(st_string , SCHED_KW_DEFAULT_ITEM) == 0)
    return DEFAULT_STATUS;
  else if( strcmp(st_string, ST_OPEN_STRING) == 0)
    return OPEN; 
  else if( strcmp(st_string, ST_STOP_STRING) == 0)
    return STOP; 
  else if( strcmp(st_string, ST_SHUT_STRING) == 0)
    return SHUT; 
  else
  {
    util_abort("%s: Could not recognize %s as a well status.\n", __func__, st_string);
    return 0;
  }
}



static cm_flag_type get_cm_flag_from_string(const char * cm_string)
{
  if(     strcmp(cm_string, CM_ORAT_STRING) == 0)
    return ORAT;
  else if(strcmp(cm_string, CM_WRAT_STRING) == 0)
    return WRAT;
  else if(strcmp(cm_string, CM_GRAT_STRING) == 0)
    return GRAT;
  else if(strcmp(cm_string, CM_LRAT_STRING) == 0)
    return LRAT;
  else if(strcmp(cm_string, CM_RESV_STRING) == 0)
    return RESV;
  else
  {
    util_abort("%s: Could not recognize %s as a control mode.\n", __func__, cm_string);
    return 0;
  }
}




static wconhist_well_type * wconhist_well_alloc_empty()
{
  wconhist_well_type * well = util_malloc(sizeof * well, __func__);
  well->name = NULL;
  return well;
}



static void wconhist_well_free(wconhist_well_type * well)
{
  free(well->name);
  free(well);
}



static void wconhist_well_free__(void * well)
{
  wconhist_well_free( (wconhist_well_type *) well);
}



static void wconhist_well_fprintf(const wconhist_well_type * well, FILE * stream)
{
  fprintf(stream, "  ");
  sched_util_fprintf_qst(well->def[0],  well->name                 , 8,     stream);
  sched_util_fprintf_qst(well->def[1],  get_st_string(well->status), 4,     stream);
  sched_util_fprintf_qst(well->def[2],  get_cm_string(well->cmode) , 4,     stream);
  sched_util_fprintf_dbl(well->def[3],  well->orat                 , 11, 3, stream);
  sched_util_fprintf_dbl(well->def[4],  well->wrat                 , 11, 3, stream);
  sched_util_fprintf_dbl(well->def[5],  well->grat                 , 11, 3, stream);
  sched_util_fprintf_int(well->def[6],  well->vfptable             , 4 ,    stream);
  sched_util_fprintf_dbl(well->def[7],  well->alift                , 11, 3, stream);
  sched_util_fprintf_dbl(well->def[8],  well->thp                  , 11, 3, stream);
  sched_util_fprintf_dbl(well->def[9] , well->bhp                  , 11, 3, stream);
  sched_util_fprintf_dbl(well->def[10], well->wgrat                , 11, 3, stream);
  fprintf(stream, "/\n");
}






static wconhist_well_type * wconhist_well_alloc_from_tokens(const stringlist_type * line_tokens ) {
  wconhist_well_type * well = wconhist_well_alloc_empty();
  sched_util_init_default( line_tokens , well->def );
  
  well->name  = util_alloc_string_copy(stringlist_iget(line_tokens, 0));

  if(!well->def[1])
    well->status = get_st_flag_from_string(stringlist_iget(line_tokens , 1));

  if(!well->def[2])
    well->cmode = get_cm_flag_from_string(stringlist_iget(line_tokens , 2));

  well->orat      = sched_util_atof(stringlist_iget(line_tokens , 3)); 
  well->wrat      = sched_util_atof(stringlist_iget(line_tokens , 4)); 
  well->grat      = sched_util_atof(stringlist_iget(line_tokens , 5)); 
  well->vfptable  = sched_util_atoi(stringlist_iget(line_tokens , 6));
  well->alift     = sched_util_atof(stringlist_iget(line_tokens , 7));
  well->thp       = sched_util_atof(stringlist_iget(line_tokens , 8));
  well->bhp       = sched_util_atof(stringlist_iget(line_tokens , 9));
  well->wgrat     = sched_util_atof(stringlist_iget(line_tokens , 10));

  return well;
}






static hash_type * wconhist_well_export_obs_hash(const wconhist_well_type * well)
{
  hash_type * obs_hash = hash_alloc();

  if(!well->def[3])
    hash_insert_double(obs_hash, "WOPR", well->orat);

  if(!well->def[4])
    hash_insert_double(obs_hash, "WWPR", well->wrat);

  if(!well->def[5])
    hash_insert_double(obs_hash, "WGPR", well->grat);
  
  if(!well->def[8])
    hash_insert_double(obs_hash, "WTHP", well->thp);
  
  if(!well->def[9])
    hash_insert_double(obs_hash, "WBHP", well->bhp);
  
  if(!well->def[10])
    hash_insert_double(obs_hash, "WWGPR", well->wgrat);

  // Water cut.
  if(!well->def[3] && !well->def[4])
  {
    double wct;
    if(well->orat + well->wrat > 0.0)
      wct = well->wrat / (well->orat + well->wrat);
    else
      wct = 0.0;

    hash_insert_double(obs_hash, "WWCT", wct);
  }

  // Gas oil ratio.
  if(!well->def[3] && !well->def[5])
  {
    double gor;
    if(well->orat > 0.0)
    {
      gor = well->grat / well->orat;
      hash_insert_double(obs_hash, "WGOR", gor);
    }
  }

  return obs_hash;
}


static void sched_kw_wconhist_add_well( sched_kw_wconhist_type * kw , wconhist_well_type * well) {
  vector_append_owned_ref(kw->wells, well, wconhist_well_free__);
}




static sched_kw_wconhist_type * sched_kw_wconhist_alloc_empty()
{
  sched_kw_wconhist_type * kw = util_malloc(sizeof * kw, __func__);
  UTIL_TYPE_ID_INIT( kw , SCHED_KW_WCONHIST_ID );
  kw->wells     = vector_alloc_new();
  return kw;
}



sched_kw_wconhist_type * sched_kw_wconhist_safe_cast( void * arg ) {
  sched_kw_wconhist_type * kw = (sched_kw_wconhist_type * ) arg;
  if (kw->__type_id == SCHED_KW_WCONHIST_ID)
    return kw;
  else {
    util_abort("%s: runtime cast failed \n",__func__);
    return NULL;
  }
}



/***********************************************************************/



sched_kw_wconhist_type * sched_kw_wconhist_alloc(const stringlist_type * tokens , int * token_index ) {
  sched_kw_wconhist_type * kw = sched_kw_wconhist_alloc_empty();
  int eokw                    = false;
  do {
    stringlist_type * line_tokens = sched_util_alloc_line_tokens( tokens , false , WCONHIST_NUM_KW , token_index );
    if (line_tokens == NULL)
      eokw = true;
    else {
      wconhist_well_type * well = wconhist_well_alloc_from_tokens( line_tokens );
      sched_kw_wconhist_add_well( kw , well );
      stringlist_free( line_tokens );
    } 
    
  } while (!eokw);
  return kw;
}


void sched_kw_wconhist_free(sched_kw_wconhist_type * kw)
{
  vector_free(kw->wells);
  free(kw);
}



void sched_kw_wconhist_fprintf(const sched_kw_wconhist_type * kw, FILE * stream)
{
  int size = vector_get_size(kw->wells);

  fprintf(stream, "WCONHIST\n");
  for(int i=0; i<size; i++)
  {
    const wconhist_well_type * well = vector_iget_const(kw->wells, i);
    wconhist_well_fprintf(well, stream);
  }
  fprintf(stream,"/\n\n");
}


/***********************************************************************/



hash_type * sched_kw_wconhist_alloc_well_obs_hash(const sched_kw_wconhist_type * kw)
{
  hash_type * well_hash = hash_alloc();

  int num_wells = vector_get_size(kw->wells);
  
  for(int well_nr=0; well_nr<num_wells; well_nr++)
  {
    wconhist_well_type * well = vector_iget(kw->wells, well_nr);
    hash_type * obs_hash = wconhist_well_export_obs_hash(well);
    hash_insert_hash_owned_ref(well_hash, well->name, obs_hash, hash_free__);
  }

  return well_hash;
}

sched_kw_wconhist_type * sched_kw_wconhist_copyc(const sched_kw_wconhist_type * kw) {
  util_abort("%s: not implemented ... \n",__func__);
  return NULL;
}



/***********************************************************************/
/* Functions exported for the sched_file_update api.                   */



/** Will return NULL if the well is not present. */
static wconhist_well_type * sched_kw_wconhist_get_well( const sched_kw_wconhist_type * kw , const char * well_name) {
  int size = vector_get_size(kw->wells);
  wconhist_well_type * well = NULL;
  int index = 0;
  do {
    wconhist_well_type * iwell = vector_iget( kw->wells , index);
    if (strcmp( well_name , iwell->name ) == 0) 
      well = iwell;
    
    index++;
  } while ((well == NULL) && (index < size));
  return well;
}


/*****************************************************************/
/* ORAT functions                                                */

double sched_kw_wconhist_get_orat( sched_kw_wconhist_type * kw , const char * well_name) {
  wconhist_well_type * well = sched_kw_wconhist_get_well( kw , well_name );
  if (well != NULL)
    return well->orat;
  else
    return -1;
}

void sched_kw_wconhist_scale_orat( sched_kw_wconhist_type * kw , const char * well_name, double factor) {
  wconhist_well_type * well = sched_kw_wconhist_get_well( kw , well_name );
  if (well != NULL)
    well->orat *= factor;
}

void sched_kw_wconhist_shift_orat( sched_kw_wconhist_type * kw , const char * well_name, double shift_value) {
  wconhist_well_type * well = sched_kw_wconhist_get_well( kw , well_name );
  if (well != NULL) {
    well->orat += shift_value;
    if (well->orat < 0)
      well->orat = 0;
  }
}

void sched_kw_wconhist_set_orat( sched_kw_wconhist_type * kw , const char * well_name , double orat) {
  wconhist_well_type * well = sched_kw_wconhist_get_well( kw , well_name );
  if (well != NULL)
    well->orat = orat;
}

/*****************************************************************/


bool sched_kw_wconhist_has_well( const sched_kw_wconhist_type * kw , const char * well_name) {
  wconhist_well_type * well = sched_kw_wconhist_get_well( kw , well_name );
  if (well == NULL)
    return false;
  else
    return true;
}




KW_IMPL(wconhist)
