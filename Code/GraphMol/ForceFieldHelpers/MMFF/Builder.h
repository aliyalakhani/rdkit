//
//  Copyright (C) 2013 Paolo Tosco
//
//  Copyright (C) 2004-2006 Rational Discovery LLC
//
//   @@ All Rights Reserved @@
//  This file is part of the RDKit.
//  The contents are covered by the terms of the BSD license
//  which is included in the file license.txt, found at the root
//  of the RDKit source tree.
//
#ifndef _RD_MMFFBUILDER_H_
#define _RD_MMFFBUILDER_H_

#include <vector>
#include <string>
#include <boost/shared_array.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/once.hpp>
#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/cstdint.hpp>
#include <ForceField/ForceField.h>


#ifdef RDK_BUILD_WITH_OPENMM
namespace OpenMM {
class MMFFBondForce;
class MMFFAngleForce;
class MMFFStretchBendForce;
class MMFFTorsionForce;
class MMFFOutOfPlaneBendForce;
class MMFFVdwForce;
class CustomNonbondedForce;
class MMFFNonbondedForce;
}
#else
typedef ForceFields::ForceField ForceFields::MMFF::OpenMMForceField;
#endif

namespace ForceFields {
namespace MMFF {
class MMFFBond;
class MMFFAngle;
class MMFFProp;
class MMFFStbn;
class MMFFTor;
class MMFFOop;
class MMFFVdWRijstarEps;
}
}

namespace RDKit {
class ROMol;
namespace MMFF {

#ifdef RDK_BUILD_WITH_OPENMM
class OpenMMForceField : public ForceFields::OpenMMForceField {
  public:
    OpenMMForceField(bool forceLoadPlugins = false,
      const std::string &pluginsDir = std::string());
    void addBondStretchContrib(unsigned int idx1, unsigned int idx2,
      const ForceFields::MMFF::MMFFBond *mmffBondParams);
    void addAngleBendContrib(unsigned int idx1, unsigned int idx2,
      unsigned int idx3, const ForceFields::MMFF::MMFFAngle *mmffAngleParams,
      const ForceFields::MMFF::MMFFProp *mmffPropParamsCentralAtom);
    void addStretchBendContrib(unsigned int idx1, unsigned int idx2,
      unsigned int idx3, const ForceFields::MMFF::MMFFStbn *mmffStbnParams,
      const ForceFields::MMFF::MMFFAngle *mmffAngleParams,
      const ForceFields::MMFF::MMFFBond *mmffBondParams1,
      const ForceFields::MMFF::MMFFBond *mmffBondParams2);
    void addTorsionAngleContrib(unsigned int idx1, unsigned int idx2,
      unsigned int idx3, unsigned int idx4,
      const ForceFields::MMFF::MMFFTor *mmffTorParams);
    void addOopBendContrib(unsigned int idx1, unsigned int idx2, unsigned int idx3,
      unsigned int idx4, const ForceFields::MMFF::MMFFOop *mmffOopParams);
    void addNonbondedContrib(unsigned int idx, double charge,
      const ForceFields::MMFF::MMFFVdW *mmffVdWParams);
    void addNonbondedExclusionsAndExceptions(
      const std::vector<std::pair<int, int> > &exclusions,
      const std::vector<std::pair<int, int> > &bonds);
    void addVdWContrib(unsigned int idx,
      const ForceFields::MMFF::MMFFVdW *mmffVdWParams, std::vector<int> &exclusions);
    void addEleContrib(unsigned int idx, double charge, boost::uint8_t dielModel,
      double dielConst, std::vector<int> &exclusions);
    void addEleContrib1_4(unsigned int idx, double charge, boost::uint8_t dielModel,
      double dielConst, std::set<int> &partners1_4);
    const std::vector<std::string>& loadedPlugins() {
      return d_loadedPlugins;
    }
    const std::vector<std::string>& failedPlugins() {
      return d_failedPlugins;
    }
    OpenMM::MMFFBondForce *getBondStretchForce() {
      return d_bondStretchForce;
    }
    OpenMM::MMFFAngleForce *getAngleBendForce() {
      return d_angleBendForce;
    }
    OpenMM::MMFFStretchBendForce *getStretchBendForce() {
      return d_stretchBendForce;
    }
    OpenMM::MMFFTorsionForce *getTorsionAngleForce() {
      return d_torsionAngleForce;
    }
    OpenMM::MMFFOutOfPlaneBendForce *getOopBendForce() {
      return d_oopBendForce;
    }
    OpenMM::MMFFNonbondedForce *getNonbondedForce() {
      return d_nonbondedForce;
    }
    OpenMM::MMFFVdwForce *getVdWForce() {
      return d_vdWForce;
    }
    OpenMM::CustomNonbondedForce *getEleForce() {
      return d_eleForce;
    }
    OpenMM::CustomNonbondedForce *getEleForce1_4() {
      return d_eleForce1_4;
    }
    void setCutoffDistance(double distance);
    void setNonbondedPeriodic(bool periodic);
  private:
    std::vector<std::string> d_loadedPlugins;
    std::vector<std::string> d_failedPlugins;
    OpenMM::MMFFBondForce *d_bondStretchForce;
    OpenMM::MMFFAngleForce *d_angleBendForce;
    OpenMM::MMFFStretchBendForce *d_stretchBendForce;
    OpenMM::MMFFTorsionForce *d_torsionAngleForce;
    OpenMM::MMFFOutOfPlaneBendForce *d_oopBendForce;
    OpenMM::MMFFNonbondedForce *d_nonbondedForce;
    OpenMM::MMFFVdwForce *d_vdWForce;
    OpenMM::CustomNonbondedForce *d_eleForce;
    OpenMM::CustomNonbondedForce *d_eleForce1_4;
};
#endif

class MMFFMolProperties;

//! Builds and returns a MMFF force field for a molecule
/*!

  \param mol              the molecule to use
  \param nonBondedThresh  the threshold to be used in adding non-bonded terms
                          to the force field. Any non-bonded contact whose
  current
                    distance is greater than \c nonBondedThresh * the minimum
  value
                    for that contact will not be included.
  \param confId     the optional conformer id, if this isn't provided, the
  molecule's
                    default confId will be used.
  \param ignoreInterfragInteractions if true, nonbonded terms will not be added
  between
                                     fragments

  \return the new force field. The client is responsible for free'ing this.
*/
ForceFields::ForceField *constructForceField(
    ROMol &mol, double nonBondedThresh = 100.0, int confId = -1,
    bool ignoreInterfragInteractions = true);

//! Builds and returns a MMFF force field for a molecule
/*!

  \param mol        the molecule to use
  \param mmffMolProperties        pointer to a MMFFMolProperties object
  \param nonBondedThresh  the threshold to be used in adding non-bonded terms
                    to the force field. Any non-bonded contact whose current
                    distance is greater than \c nonBondedThresh * the minimum
  value
                    for that contact will not be included.
  \param confId     the optional conformer id, if this isn't provided, the
  molecule's
                    default confId will be used.
  \param ignoreInterfragInteractions if true, nonbonded terms will not be added
  between
                                     fragments

  \return the new force field. The client is responsible for free'ing this.
*/
ForceFields::ForceField *constructForceField(
  ROMol &mol, MMFFMolProperties *mmffMolProperties,
  double nonBondedThresh = 100.0, int confId = -1,
  bool ignoreInterfragInteractions = true);

ForceFields::ForceField *constructForceField(
  ROMol &mol, MMFFMolProperties *mmffMolProperties,
  int ffOpts, double nonBondedThresh = 100.0, int confId = -1,
  bool ignoreInterfragInteractions = true);

#ifdef RDK_BUILD_WITH_OPENMM
OpenMMForceField *constructOpenMMForceField(ROMol &mol,
  double nonBondedThresh = 100.0,
  int confId = -1, bool ignoreInterfragInteractions = true);

OpenMMForceField *constructOpenMMForceField(ROMol &mol,
  MMFFMolProperties *mmffMolProperties, double nonBondedThresh = 100.0,
  int confId = -1, bool ignoreInterfragInteractions = true);
#endif

namespace Tools {
class DefaultTorsionBondSmarts : private boost::noncopyable
{
public:
  static const std::string &string() {
    return ds_string;
  }
  static const ROMol *query();
private:
  DefaultTorsionBondSmarts() {}
  static void create();
  static const std::string ds_string;
  static boost::scoped_ptr<const ROMol> ds_instance;
  static boost::once_flag ds_flag;
};

enum { RELATION_1_2 = 0, RELATION_1_3 = 1, RELATION_1_4 = 2, RELATION_1_X = 3 };
// these functions are primarily exposed so they can be tested.
unsigned int twoBitCellPos(unsigned int nAtoms, int i, int j);
void setTwoBitCell(boost::shared_array<boost::uint8_t> &res, unsigned int pos,
                   boost::uint8_t value);
boost::uint8_t getTwoBitCell(boost::shared_array<boost::uint8_t> &res,
                             unsigned int pos);
boost::shared_array<boost::uint8_t> buildNeighborMatrix(const ROMol &mol);
void addBonds(const ROMol &mol, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field, int ffOpts);
void addBonds(const ROMol &mol, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field);
void addAngles(const ROMol &mol, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field, int ffOpts);
void addAngles(const ROMol &mol, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field);
void addStretchBend(const ROMol &mol, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field, int ffOpts);
void addStretchBend(const ROMol &mol, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field);
void addOop(const ROMol &mol, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field, int ffOpts);
void addOop(const ROMol &mol, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field);
void addTorsions(const ROMol &mol, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field, int ffOpts,
  const std::string &torsionBondSmarts = DefaultTorsionBondSmarts::string());
void addTorsions(const ROMol &mol, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field,
  const std::string &torsionBondSmarts = DefaultTorsionBondSmarts::string());
void addVdW(const ROMol &mol, int confId, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field, int ffOpts,
  boost::shared_array<boost::uint8_t> neighborMatrix,
  double nonBondedThresh = 100.0,
  bool ignoreInterfragInteractions = true);
void addVdW(const ROMol &mol, int confId, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field,
  boost::shared_array<boost::uint8_t> neighborMatrix,
  double nonBondedThresh = 100.0,
  bool ignoreInterfragInteractions = true);
void addEle(const ROMol &mol, int confId, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field, int ffOpts,
  boost::shared_array<boost::uint8_t> neighborMatrix,
  double nonBondedThresh = 100.0,
  bool ignoreInterfragInteractions = true);
void addEle(const ROMol &mol, int confId, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field,
  boost::shared_array<boost::uint8_t> neighborMatrix,
  double nonBondedThresh = 100.0,
  bool ignoreInterfragInteractions = true);
#ifdef RDK_BUILD_WITH_OPENMM
void addNonbonded(const ROMol &mol, int confId, MMFFMolProperties *mmffMolProperties,
  ForceFields::ForceField *field,
  boost::shared_array<boost::uint8_t> neighborMatrix,
  double nonBondedThresh, bool ignoreInterfragInteractions);
#endif
}
}
}

#endif
