/* -*- c++ -*- ----------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#ifdef FIX_CLASS

FixStyle(ssa_tsdpd/bvf,FixSsaTsdpdBvf)

#else

#ifndef LMP_FIX_SSA_TSDPD_BVF_H
#define LMP_FIX_SSA_TSDPD_BVF_H

#include "fix.h"
#include "random_mars.h"

namespace LAMMPS_NS {

class FixSsaTsdpdBvf : public Fix {
 public:
  FixSsaTsdpdBvf(class LAMMPS *, int, char **);
  int setmask();
  virtual void init();
  virtual void setup_pre_force(int);
  virtual void initial_integrate(int);
  virtual void final_integrate();
  void reset_dt();

 private:
  class NeighList *list;
 protected:
  double dtv,dtf;
  double *step_respa;
  int mass_require;
  class Pair *pair;
  unsigned int seed;
  class RanMars *random;
};

}

#endif
#endif
