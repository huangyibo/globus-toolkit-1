
/********************************************************************
 *
 * This file implements the symboltable_t type, 
 * a lightweight chaining symboltable
 *
 ********************************************************************/

#include "globus_symboltable.h"
#include "globus_list.h"
#include "globus_hashtable.h"
#include "globus_libc.h"


void *globus_symboltable_lookup (globus_symboltable_t *table, 
				 void *symbol)
{
  globus_list_t * scope_iter;

  scope_iter = table->scopes;

  while ( ! globus_list_empty (scope_iter) ) {
    void * datum;

    if ( (datum = globus_hashtable_lookup (((globus_hashtable_t *)
					    globus_list_first (scope_iter)),
					   symbol))
	 != GLOBUS_NULL ) 
      return datum;

    scope_iter = globus_list_rest (scope_iter);
  }

  return GLOBUS_NULL;
}

int globus_symboltable_insert (globus_symboltable_t *table, 
			       void *symbol, 
			       void *datum)
{
  if ( globus_list_empty (table->scopes) )
    return 1;
  else
    return globus_hashtable_insert (((globus_hashtable_t *)
				     globus_list_first (table->scopes)),
				    symbol,
				    datum);
}

void *globus_symboltable_remove (globus_symboltable_t *table, 
				 void *symbol)
{
  if ( globus_list_empty (table->scopes) )
    return GLOBUS_NULL;
  else
    return globus_hashtable_remove (((globus_hashtable_t *)
				     globus_list_first (table->scopes)),
				    symbol);
}


int
globus_symboltable_create_scope (globus_symboltable_t *table)
{
  int err;
  globus_hashtable_t * new_scope;

  new_scope = globus_malloc (sizeof(globus_hashtable_t));
  assert (new_scope!=GLOBUS_NULL);

  err = globus_hashtable_init (new_scope, 16 /* reasonable default */,
			       table->hash_func, table->keyeq_func);
  assert (!err);

  err = globus_list_insert (&(table->scopes), 
			    (void *) new_scope);
  assert (!err);

  return 0;
}

int 
globus_symboltable_remove_scope (globus_symboltable_t *table)
{
  if ( globus_list_empty (table->scopes) ) 
    return 1;
  else {
    int err;
    globus_hashtable_t * old_scope;

    old_scope = ((globus_hashtable_t *)
		 globus_list_first (table->scopes));

    globus_list_remove (&(table->scopes), table->scopes);

    err = globus_hashtable_destroy (old_scope);
    assert (!err);

    globus_libc_free(old_scope);

    return 0;
  }
}


int globus_symboltable_init (globus_symboltable_t *table,
			     globus_hashtable_hash_func_t hash_func,
			     globus_hashtable_keyeq_func_t keyeq_func)
{
  if (table==NULL) return 1;

  table->scopes = NULL;
  table->hash_func = hash_func;
  table->keyeq_func = keyeq_func;

  return 0;
}

int globus_symboltable_destroy (globus_symboltable_t *table)
{
  assert (table!=GLOBUS_NULL);

  while ( ! globus_list_empty (table->scopes) ) {
    int err;
    globus_hashtable_t * old_scope;

    old_scope = ((globus_hashtable_t *)
		 globus_list_first (table->scopes));

    globus_list_remove (&(table->scopes), table->scopes);

    err = globus_hashtable_destroy (old_scope);
    assert (!err);
  }

  return 0;
}