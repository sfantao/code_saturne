/* VERS */

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2017 EDF S.A.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include "bft_mem.h"
#include "bft_error.h"
#include "bft_printf.h"

#include "cs_base.h"
#include "cs_gui_util.h"
#include "cs_math.h"
#include "cs_selector.h"
#include "cs_parameters.h"

#include "cs_mesh.h"

#include "cs_lagr.h"
#include "cs_lagr_new.h"
#include "cs_lagr_tracking.h"
#include "cs_lagr_prototypes.h"

/*============================================================================
 * User function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief Define particle boundary conditions.
 *
 * This is used definition of for inlet and of the other boundaries
 *
 * Boundary faces may be selected using the
 * \ref cs_selector_get_b_face_list function.
 *
 * \param[in] bc_type    type of the boundary faces
 */
/*----------------------------------------------------------------------------*/

void
cs_user_lagr_boundary_conditions(const int  bc_type[])
{

}

/*----------------------------------------------------------------------------*/

END_C_DECLS