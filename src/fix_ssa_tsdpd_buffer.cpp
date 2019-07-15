/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */
#include "mpi.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "fix_ssa_tsdpd_buffer.h"
#include "atom.h"
#include "force.h"
#include "update.h"
#include "error.h"
#include "comm.h"
#include "domain.h"
#include "memory.h"
#include "iostream"

using namespace LAMMPS_NS;
using namespace FixConst;

/* ---------------------------------------------------------------------- */

FixSsaTsdpdBuffer::FixSsaTsdpdBuffer(LAMMPS *lmp, int narg, char **arg) :
  Fix(lmp, narg, arg)
{
  if (strcmp(style,"ssa_tsdpd_buffer") != 0 && narg < 4)
    error->all(FLERR,"Illegal fix SsaTsdpdBuffer command, first error.");


  //ssa_case = 0;
  tsdpd_case = 0;
  velocity_case = 0;
  x_case = 0;
  y_case = 0;
  z_case = 0;

  int arg_index = 3;
 
  if (strcmp(arg[arg_index],"ssa") == 0)  ssa_case = 1;
  else if (strcmp(arg[arg_index],"tsdpd") == 0)  tsdpd_case = 1;
  else if (strcmp(arg[arg_index],"velocity") == 0) velocity_case = 1;
  else if (strcmp(arg[arg_index],"density") == 0) density_case = 1;
  else error->all(FLERR,"Illegal argument[3]. Choose <ssa>, <tsdpd>, <velocity> or <density>");
  
  arg_index++;

  if (strcmp(arg[arg_index],"x") == 0)  x_case = 1;
  else if (strcmp(arg[arg_index],"y") == 0)  y_case = 1;
  //else if (strcmp(arg[arg_index],"z") == 0) z_case = 1;
  else error->all(FLERR,"Illegal argument[4]. Choose <x> or <y>");

  arg_index++;

  step = atoi(arg[arg_index++]);

  //if (ssa_case==1) {
  //  ctype = atoi(arg[arg_index++]);
  //  if(ctype > atom->num_ssa_species) 
  //      error->all(FLERR,"Illegal fix ssa_tsdpd_buffer command: species id > num_ssa_species.\n");   
  //}
  if (tsdpd_case==1) {
     ctype = atoi(arg[arg_index++]);
     if(ctype > atom->num_sdpd_species)  
        error->all(FLERR,"Illegal fix ssa_tsdpd_buffer command: species id > num_tsdpd_species.\n");   
  }
  else if (velocity_case==1) { 
     vtype = atoi(arg[arg_index++]);
     if(vtype > 3)  
        error->all(FLERR,"Illegal fix ssa_tsdpd_buffer command: velocity id must be either 1 (vx), 2 (vy)  or 3 (vz).\n");   
  }
  else if (density_case==1) { 
     densitytype = atoi(arg[arg_index++]);
  }


  if (narg != 12 ) error->all(FLERR,"Illegal fix ssa_tsdpd_buffer command, index1");
  center[0] = atof( arg[arg_index++] );
  center[1] = atof( arg[arg_index++] );
  length = atof( arg[arg_index++] );
  width  = atof( arg[arg_index++] );
  if (tsdpd_case == 1) value  = atof( arg[arg_index++] );
  else if (ssa_case == 1) value_int = int(atof( arg[arg_index++] ));
  else if (velocity_case == 1) value = atof( arg[arg_index++] );
  else if (density_case == 1) value = atof( arg[arg_index++] );
  else error->all(FLERR,"Illegal fix ssa_tsdpd_buffer command, index error.");

  MPI_Barrier(world);

}

/* ---------------------------------------------------------------------- */

FixSsaTsdpdBuffer::~FixSsaTsdpdBuffer()
{
}

/* ---------------------------------------------------------------------- */

int FixSsaTsdpdBuffer::setmask()
{
  int mask = 0;
  mask |= POST_INTEGRATE;
  mask |= END_OF_STEP;
  return mask;
}

/* ---------------------------------------------------------------------- */

void FixSsaTsdpdBuffer::init()
{

}

/* ---------------------------------------------------------------------- */
void FixSsaTsdpdBuffer::post_integrate()
{
  int *mask = atom->mask;
  int nlocal = atom->nlocal;
  double **x = atom->x;
  double **v = atom->vest;
  double **C = atom->C;
  double **Q = atom->Q;
  double *rho = atom->rho;
  //int **Cd = atom->Cd;

  if (igroup == atom->firstgroup) nlocal = atom->nfirst;

  double drx, dry, rsq, xo, xL, yo, yL, B, phi;

  if(update->ntimestep > step) 
  for (int i = 0; i < nlocal; i++) {
    if (mask[i] & groupbit) {

      // If buffer zone extends along the x-direction
      if (x_case == 1) {
        drx = x[i][0] - center[0];
        dry = x[i][1] - center[1];

        // If inside buffer region
      	if(fabs(drx) < length && fabs(dry) < width){
          B = 1.0;
          xo = center[0] - length;
          xL = center[0] + length;
          phi = (x[i][0]-xo)/(xL-xo);
          //phi = 0.5 * (1.0 - tanh(8.0 - 16.0*phi)); // for tanh vortex stretching function
          phi = phi*phi*phi;   // for cubic vortex stretching function
    	  if (tsdpd_case==1)  C[i][ctype] = C[i][ctype] - phi*(C[i][ctype] - value);
          else if (velocity_case==1) v[i][vtype] = v[i][vtype] - phi*(v[i][vtype] - value);
          //else if (ssa_case==1)  Cd[i][ctype] = Cd[i][ctype] - ceil(phi*(Cd[i][ctype] - value_int));
        }
      }

      // If buffer zone extends along the y-direction
      else if (y_case == 1) {
        drx = x[i][0] - center[0];
        dry = x[i][1] - center[1];

        // If inside buffer region
      	if(fabs(drx) < length && fabs(dry) < width){
          B = 1.0;
          yo = center[1] - width;
          yL = center[1] + width;
          phi = (x[i][1]-yo)/(yL-yo);
          phi = 0.5 * (1.0 - tanh(8.0 - 16.0*phi)); 
    	  if (tsdpd_case==1) C[i][ctype] = C[i][ctype] - phi*(C[i][ctype] - value);
          else if (velocity_case==1) v[i][vtype] = v[i][vtype] - phi*(v[i][vtype] - value);
          //else if (ssa_case==1)  Cd[i][ctype] = Cd[i][ctype] - ceil(phi*(Cd[i][ctype] - value_int));
        }
      }

/*
      else if (z_case == 1) {
      }
*/

    }
  }
}
/* ---------------------------------------------------------------------- */
void FixSsaTsdpdBuffer::end_of_step()
{
  int *mask = atom->mask;
  int nlocal = atom->nlocal;
  double **x = atom->x;
  double **v = atom->vest;
  double **C = atom->C;
  double **Q = atom->Q;
  double *rho = atom->rho;

  if (igroup == atom->firstgroup) nlocal = atom->nfirst;

  double drx, dry, rsq, xo, xL, yo, yL, B, phi;

  if(update->ntimestep > step) 
  for (int i = 0; i < nlocal; i++) {
    if (mask[i] & groupbit) {

      // If buffer zone extends along the x-direction
      if (x_case == 1) {
        drx = x[i][0] - center[0];
        dry = x[i][1] - center[1];

        // If inside buffer region
      	if(fabs(drx) < length && fabs(dry) < width){
          xo = center[0] - length;
          xL = center[0] + length;
          phi = (x[i][0]-xo)/(xL-xo);
          // phi = 0.5 * (1.0 - tanh(8.0 - 16.0*phi)); // for tanh vortex stretching function
	  phi = phi*phi*phi; // for cubic vortex stretching function
          if (density_case==1) rho[i] = rho[i] - phi*(rho[i] - value);
        }
      }

      // If buffer zone extends along the y-direction
      else if (y_case == 1) {
        drx = x[i][0] - center[0];
        dry = x[i][1] - center[1];

        // If inside buffer region
      	if(fabs(drx) < length && fabs(dry) < width){
          yo = center[1] - width;
          yL = center[1] + width;
          phi = (x[i][1]-yo)/(yL-yo);
          phi = 0.5 * (1.0 - tanh(8.0 - 16.0*phi)); 
          if (density_case==1)  rho[i] = rho[i] - phi*(rho[i] - value);
        }
      }

/*
      else if (z_case == 1) {
      }
*/

    }
  }
}
/* ---------------------------------------------------------------------- */

