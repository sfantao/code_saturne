/*============================================================================
 * Extract information from lagrangian particles.
 *============================================================================*/

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2013 EDF S.A.

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at your option) any later
  version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
  Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

/*----------------------------------------------------------------------------*/

#include "cs_defs.h"

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include "bft_mem.h"
#include "bft_printf.h"

#include "cs_base.h"
#include "cs_field.h"
#include "cs_log.h"
#include "cs_mesh.h"
#include "cs_parall.h"
#include "cs_lagr_tracking.h"
#include "cs_prototypes.h"
#include "cs_selector.h"
#include "cs_timer.h"
#include "cs_time_step.h"

/*----------------------------------------------------------------------------
 * Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_lagr_extract.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Additional doxygen documentation
 *============================================================================*/

/*!
  \file cs_lagr_extract.c

  \brief Extract information from lagrangian particles.
*/

/*============================================================================
 * Type definitions
 *============================================================================*/

/*! \cond DOXYGEN_SHOULD_SKIP_THIS */

/*============================================================================
 * Static global variables
 *============================================================================*/

/*============================================================================
 * Private function definitions
 *============================================================================*/

/*============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief Get the local number of particles.
 *
 * \return  current number of particles.
 */
/*----------------------------------------------------------------------------*/

cs_lnum_t
cs_lagr_get_n_particles(void)
{
  cs_lnum_t retval = 0;

  cs_lagr_particle_set_t  *p_set = NULL;
  cs_lagr_get_particle_sets(&p_set, NULL);
  if (p_set != NULL)
    retval = p_set->n_particles;

  return retval;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Extract a list of particles using an optional cell filter and
 *        statistical density filter.
 *
 * The output array must have been allocated by the caller and be of
 * sufficient size.
 *
 * \param[in]   n_cells          number of cells in filter
 * \param[in]   cell_list        optional list of containing cells filter
 *                               (1 to n numbering)
 * \param[in]   density          if < 1, fraction of particles to select
 * \param[out]  n_particles      number of selected particles, or NULL
 * \param[out]  particle_list    particle_list (1 to n numbering), or NULL
 */
/*----------------------------------------------------------------------------*/

void
cs_lagr_get_particle_list(cs_lnum_t         n_cells,
                          const cs_lnum_t   cell_list[],
                          double            density,
                          cs_lnum_t        *n_particles,
                          cs_lnum_t        *particle_list)
{
  cs_lnum_t i;

  cs_lnum_t p_count = 0;
  cs_lagr_particle_set_t  *p_set = NULL;

  bool *cell_flag = NULL;

  const cs_mesh_t *mesh = cs_glob_mesh;
  const double r_max = RAND_MAX;

  cs_lagr_get_particle_sets(&p_set, NULL);

  /* Case where we have a filter */

  if (n_cells < mesh->n_cells) {

    /* Build cel filter */

    BFT_MALLOC(cell_flag, mesh->n_cells, bool);

    for (i = 0; i < mesh->n_cells; i++)
      cell_flag[i] = false;

    if (cell_list != NULL) {
      for (i = 0; i < n_cells; i++)
        cell_flag[cell_list[i] - 1] = true;
    }
    else {
      for (i = 0; i < n_cells; i++)
        cell_flag[i] = true;
    }

  }

  /* Loop on particles */

  for (i = 0; i < p_set->n_particles; i++) {

    /* If density < 1, randomly select which particles are added */

    if (density < 1) {
      double r = (double)rand() / r_max;
      if (r > density)
        continue;
    }

    /* Check for filter cell */

    if (cell_flag != NULL) {
      cs_lnum_t  cell_id = CS_ABS(p_set->particles[i].cur_cell_num) - 1;
      if (cell_flag[cell_id] == false)
        continue;
    }

    if (particle_list != NULL)
      particle_list[p_count] = i+1;

    p_count += 1;

  }

  if (cell_flag != NULL)
    BFT_FREE(cell_flag);

  *n_particles = p_count;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Extract values for a set of particles.
 *
 * The output array must have been allocated by the caller and be of
 * sufficient size.
 *
 * \param[in]   particles             associated particle set
 * \param[in]   attr                  attribute whose values are required
 * \param[in]   datatype              associated value type
 * \param[in]   stride                number of values per particle
 * \param[in]   n_particles           number of particles in filter
 * \param[in]   particle_list         particle_list (1 to n numbering), or NULL
 * \param[out]  values                particle values for given attribute
 *
 * \return 0 in case of success, 1 if attribute is not present
 */
/*----------------------------------------------------------------------------*/

int
cs_lagr_get_particle_values(const cs_lagr_particle_set_t  *particles,
                            cs_lagr_attribute_t            attr,
                            cs_datatype_t                  datatype,
                            int                            stride,
                            cs_lnum_t                      n_particles,
                            const cs_lnum_t                particle_list[],
                            void                          *values)
{
  size_t j;
  cs_lnum_t i;

  size_t  extents, size;
  ptrdiff_t  displ;
  cs_datatype_t _datatype;
  int  _count;
  unsigned char *_values = values;

  assert(particles != NULL);

  cs_lagr_get_attr_info(attr, &extents, &size, &displ, &_datatype, &_count);

  if (_count == 0)
    return 1;

  /* Check consistency */

  if (datatype != _datatype || stride != _count) {
    char attr_name[32];
    const char *_attr_name = attr_name;
    if (attr < CS_LAGR_N_ATTRIBUTES)
      _attr_name = cs_lagr_attribute_name[attr];
    else {
      snprintf(attr_name, 31, "%d", (int)attr);
      attr_name[31] = '\0';
    }
    bft_error(__FILE__, __LINE__, 0,
              _("Attribute %s is of datatype %s and stride %d\n"
                "but %s and %d were requested."),
              _attr_name,
              cs_datatype_name[_datatype], _count,
              cs_datatype_name[datatype], stride);
    return 1;
  }

  /* Case where we have no filter */

  if (particle_list == NULL) {
    for (i = 0; i < n_particles; i++) {
      unsigned char *dest = _values + i*size;
      const unsigned char
        *src = (const unsigned char *)(particles->particles + i) + displ;
      for (j = 0; j < size; j++)
        dest[j] = src[j];
    }
  }

  /* Case where we have a filter list */
  else {
    for (i = 0; i < n_particles; i++) {
      cs_lnum_t p_id = particle_list[i] - 1;
      unsigned char *dest = _values + i*size;
      const unsigned char
        *src = (const unsigned char *)(particles->particles + p_id) + displ;
      for (j = 0; j < size; j++)
        dest[j] = src[j];
    }
  }

  return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Extract trajectory values joining 2 sets of particles.
 *
 * Trajectories are defined as a mesh of segments, whose start and end
 * points are copied in an interleaved manner in the segment_values array
 * (p1_old, p1_new, p2_old, p2_new, ... pn_old, pn_new).
 *
 * The output array must have been allocated by the caller and be of
 * sufficient size.
 *
 * \param[in]   particles             associated particle set
 * \param[in]   particles_prev        associated previous particle set
 * \param[in]   attr                  attribute whose values are required
 * \param[in]   datatype              associated value type
 * \param[in]   stride                number of values per particle
 * \param[in]   n_particles           number of particles in filter
 * \param[in]   particle_list         particle_list (1 to n numbering), or NULL
 * \param[out]  segment_values        particle segment values
 *
 * \return 0 in case of success, 1 if attribute is not present
 */
/*----------------------------------------------------------------------------*/

int
cs_lagr_get_trajectory_values(const cs_lagr_particle_set_t  *particles,
                              const cs_lagr_particle_set_t  *particles_prev,
                              cs_lagr_attribute_t            attr,
                              cs_datatype_t                  datatype,
                              int                            stride,
                              cs_lnum_t                      n_particles,
                              const cs_lnum_t                particle_list[],
                              void                          *segment_values)
{
  size_t j;
  cs_lnum_t i;

  size_t  extents, size;
  ptrdiff_t  displ;
  cs_datatype_t _datatype;
  int  _count;
  unsigned char *_values = segment_values;

  assert(particles != NULL);

  cs_lagr_get_attr_info(attr, &extents, &size, &displ, &_datatype, &_count);

  if (_count == 0)
    return 1;

  /* Check consistency */

  if (datatype != _datatype || stride != _count) {
    char attr_name[32];
    const char *_attr_name = attr_name;
    if (attr < CS_LAGR_N_ATTRIBUTES)
      _attr_name = cs_lagr_attribute_name[attr];
    else {
      snprintf(attr_name, 31, "%d", (int)attr);
      attr_name[31] = '\0';
    }
    bft_error(__FILE__, __LINE__, 0,
              _("Attribute %s is of datatype %s and stride %d\n"
                "but %s and %d were requested."),
              _attr_name,
              cs_datatype_name[_datatype], _count,
              cs_datatype_name[datatype], stride);
    return 1;
  }

  if (particle_list == NULL) {
    for (i = 0; i < n_particles; i++) {
      unsigned char *dest = _values + i*size*2;
      const unsigned char
        *src = (const unsigned char *)(particles->particles + i) + displ;
      const unsigned char
        *srcp = (const unsigned char *)(particles_prev->particles + i) + displ;
      for (j = 0; j < size; j++) {
        dest[j] = src[j];
        dest[j + size] = srcp[j];
      }
    }
  }

  /* Case where we have a filter list */
  else {
    for (i = 0; i < n_particles; i++) {
      cs_lnum_t p_id = particle_list[i] - 1;
      unsigned char *dest = _values + i*size*2;
      const unsigned char
        *src = (const unsigned char *)(particles->particles + p_id) + displ;
      const unsigned char
        *srcp = (const unsigned char *)(particles_prev->particles + p_id)
                + displ;
      for (j = 0; j < size; j++) {
        dest[j] = src[j];
        dest[j + size] = srcp[j];
      }
    }
  }

  return 0;
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
