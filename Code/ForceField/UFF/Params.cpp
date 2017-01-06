// $Id$
//
//  Copyright (C) 2004-2006 Rational Discovery LLC
//
//   @@ All Rights Reserved @@
//  This file is part of the RDKit.
//  The contents are covered by the terms of the BSD license
//  which is included in the file license.txt, found at the root
//  of the RDKit source tree.
//
#ifdef WIN32
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include "Params.h"

#include <iostream>
#include <sstream>
#include <RDGeneral/StreamOps.h>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/bind.hpp>
#include <Geometry/point.h>

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

namespace ForceFields {
namespace UFF {

boost::scoped_ptr<ParamCollection> ParamCollection::ds_instance;
#ifdef BOOST_THREAD_PROVIDES_ONCE_CXX11
boost::once_flag ParamCollection::ds_flag;
#else
boost::once_flag ParamCollection::ds_flag = BOOST_ONCE_INIT;
#endif

extern const std::string defaultParamData;

ParamCollection *ParamCollection::getParams(const std::string &paramData) {
#ifdef RDK_THREADSAFE_SSS
  boost::call_once(ds_flag, boost::bind(&create, paramData));
#else
  static bool created = false;
  if (!created || !paramData.empty()) {
    created = true;
    create(paramData);
  }
#endif
  return ds_instance.get();
}

void ParamCollection::create(const std::string &paramData) {
  ds_instance.reset(new ParamCollection(paramData));
}

ParamCollection::ParamCollection(const std::string &paramData) {
  std::istringstream inStream(paramData.empty() ? defaultParamData : paramData);

  std::string inLine = RDKit::getLine(inStream);
  while (!inStream.eof()) {
    if (inLine[0] != '#') {
      AtomicParams paramObj;
      boost::char_separator<char> tabSep("\t");
      tokenizer tokens(inLine, tabSep);
      tokenizer::iterator token = tokens.begin();

      std::string label = *token;
      ++token;

      paramObj.r1 = boost::lexical_cast<double>(*token);
      ++token;
      paramObj.theta0 = boost::lexical_cast<double>(*token);
      ++token;
      paramObj.theta0 = paramObj.theta0 * M_PI / 180.;

      paramObj.x1 = boost::lexical_cast<double>(*token);
      ++token;
      paramObj.D1 = boost::lexical_cast<double>(*token);
      ++token;
      paramObj.zeta = boost::lexical_cast<double>(*token);
      ++token;
      paramObj.Z1 = boost::lexical_cast<double>(*token);
      ++token;
      paramObj.V1 = boost::lexical_cast<double>(*token);
      ++token;
      paramObj.U1 = boost::lexical_cast<double>(*token);
      ++token;
      paramObj.GMP_Xi = boost::lexical_cast<double>(*token);
      ++token;
      paramObj.GMP_Hardness = boost::lexical_cast<double>(*token);
      ++token;
      paramObj.GMP_Radius = boost::lexical_cast<double>(*token);
      ++token;
      d_params[label] = paramObj;
    }
    inLine = RDKit::getLine(inStream);
  }
}
const std::string defaultParamData =
    "#Atom	r1	theta0	x1	D1	zeta	Z1	Vi	Uj	"
    "Xi	Hard	Radius\n"
    "H_	0.354	180	2.886	0.044	12	0.712	0	0	"
    "4.528	6.9452	0.371\n"
    "H_b	0.46	83.5	2.886	0.044	12	0.712	0	0	"
    "4.528	6.9452	0.371\n"
    "He4+4	0.849	90	2.362	0.056	15.24	0.098	0	0	"
    "9.66	14.92	1.3\n"
    "Li	1.336	180	2.451	0.025	12	1.026	0	2	"
    "3.006	2.386	1.557\n"
    "Be3+2	1.074	109.47	2.745	0.085	12	1.565	0	2	"
    "4.877	4.443	1.24\n"
    "B_3	0.838	109.47	4.083	0.18	12.052	1.755	0	2	"
    "5.11	4.75	0.822\n"
    "B_2	0.828	120	4.083	0.18	12.052	1.755	0	2	"
    "5.11	4.75	0.822\n"
    "C_3	0.757	109.47	3.851	0.105	12.73	1.912	2.119	2	"
    "5.343	5.063	0.759\n"
    "C_R	0.729	120	3.851	0.105	12.73	1.912	0	2	"
    "5.343	5.063	0.759\n"
    "C_2	0.732	120	3.851	0.105	12.73	1.912	0	2	"
    "5.343	5.063	0.759\n"
    "C_1	0.706	180	3.851	0.105	12.73	1.912	0	2	"
    "5.343	5.063	0.759\n"
    "N_3	0.7	106.7	3.66	0.069	13.407	2.544	0.45	2	"
    "6.899	5.88	0.715\n"
    "N_R	0.699	120	3.66	0.069	13.407	2.544	0	2	"
    "6.899	5.88	0.715\n"
    "N_2	0.685	111.2	3.66	0.069	13.407	2.544	0	2	"
    "6.899	5.88	0.715\n"
    "N_1	0.656	180	3.66	0.069	13.407	2.544	0	2	"
    "6.899	5.88	0.715\n"
    "O_3	0.658	104.51	3.5	0.06	14.085	2.3	0.018	2	"
    "8.741	6.682	0.669\n"
    "O_3_z	0.528	146	3.5	0.06	14.085	2.3	0.018	2	"
    "8.741	6.682	0.669\n"
    "O_R	0.68	110	3.5	0.06	14.085	2.3	0	2	"
    "8.741	6.682	0.669\n"
    "O_2	0.634	120	3.5	0.06	14.085	2.3	0	2	"
    "8.741	6.682	0.669\n"
    "O_1	0.639	180	3.5	0.06	14.085	2.3	0	2	"
    "8.741	6.682	0.669\n"
    "F_	0.668	180	3.364	0.05	14.762	1.735	0	2	"
    "10.874	7.474	0.706\n"
    "Ne4+4	0.92	90	3.243	0.042	15.44	0.194	0	2	"
    "11.04	10.55	1.768\n"
    "Na	1.539	180	2.983	0.03	12	1.081	0	1.25	"
    "2.843	2.296	2.085\n"
    "Mg3+2	1.421	109.47	3.021	0.111	12	1.787	0	1.25	"
    "3.951	3.693	1.5\n"
    "Al3	1.244	109.47	4.499	0.505	11.278	1.792	0	1.25	"
    "4.06	3.59	1.201\n"
    "Si3	1.117	109.47	4.295	0.402	12.175	2.323	1.225	1.25	"
    "4.168	3.487	1.176\n"
    "P_3+3	1.101	93.8	4.147	0.305	13.072	2.863	2.4	1.25	"
    "5.463	4	1.102\n"
    "P_3+5	1.056	109.47	4.147	0.305	13.072	2.863	2.4	1.25	"
    "5.463	4	1.102\n"
    "P_3+q	1.056	109.47	4.147	0.305	13.072	2.863	2.4	1.25	"
    "5.463	4	1.102\n"
    "S_3+2	1.064	92.1	4.035	0.274	13.969	2.703	0.484	1.25	"
    "6.928	4.486	1.047\n"
    "S_3+4	1.049	103.2	4.035	0.274	13.969	2.703	0.484	1.25	"
    "6.928	4.486	1.047\n"
    "S_3+6	1.027	109.47	4.035	0.274	13.969	2.703	0.484	1.25	"
    "6.928	4.486	1.047\n"
    "S_R	1.077	92.2	4.035	0.274	13.969	2.703	0	1.25	"
    "6.928	4.486	1.047\n"
    "S_2	0.854	120	4.035	0.274	13.969	2.703	0	1.25	"
    "6.928	4.486	1.047\n"
    "Cl	1.044	180	3.947	0.227	14.866	2.348	0	1.25	"
    "8.564	4.946	0.994\n"
    "Ar4+4	1.032	90	3.868	0.185	15.763	0.3	0	1.25	"
    "9.465	6.355	2.108\n"
    "K_	1.953	180	3.812	0.035	12	1.165	0	0.7	"
    "2.421	1.92	2.586\n"
    "Ca6+2	1.761	90	3.399	0.238	12	2.141	0	0.7	"
    "3.231	2.88	2\n"
    "Sc3+3	1.513	109.47	3.295	0.019	12	2.592	0	0.7	"
    "3.395	3.08	1.75\n"
    "Ti3+4	1.412	109.47	3.175	0.017	12	2.659	0	0.7	"
    "3.47	3.38	1.607\n"
    "Ti6+4	1.412	90	3.175	0.017	12	2.659	0	0.7	"
    "3.47	3.38	1.607\n"
    "V_3+5	1.402	109.47	3.144	0.016	12	2.679	0	0.7	"
    "3.65	3.41	1.47\n"
    "Cr6+3	1.345	90	3.023	0.015	12	2.463	0	0.7	"
    "3.415	3.865	1.402\n"
    "Mn6+2	1.382	90	2.961	0.013	12	2.43	0	0.7	"
    "3.325	4.105	1.533\n"
    "Fe3+2	1.27	109.47	2.912	0.013	12	2.43	0	0.7	"
    "3.76	4.14	1.393\n"
    "Fe6+2	1.335	90	2.912	0.013	12	2.43	0	0.7	"
    "3.76	4.14	1.393\n"
    "Co6+3	1.241	90	2.872	0.014	12	2.43	0	0.7	"
    "4.105	4.175	1.406\n"
    "Ni4+2	1.164	90	2.834	0.015	12	2.43	0	0.7	"
    "4.465	4.205	1.398\n"
    "Cu3+1	1.302	109.47	3.495	0.005	12	1.756	0	0.7	"
    "4.2	4.22	1.434\n"
    "Zn3+2	1.193	109.47	2.763	0.124	12	1.308	0	0.7	"
    "5.106	4.285	1.4\n"
    "Ga3+3	1.26	109.47	4.383	0.415	11	1.821	0	0.7	"
    "3.641	3.16	1.211\n"
    "Ge3	1.197	109.47	4.28	0.379	12	2.789	0.701	0.7	"
    "4.051	3.438	1.189\n"
    "As3+3	1.211	92.1	4.23	0.309	13	2.864	1.5	0.7	"
    "5.188	3.809	1.204\n"
    "Se3+2	1.19	90.6	4.205	0.291	14	2.764	0.335	0.7	"
    "6.428	4.131	1.224\n"
    "Br	1.192	180	4.189	0.251	15	2.519	0	0.7	"
    "7.79	4.425	1.141\n"
    "Kr4+4	1.147	90	4.141	0.22	16	0.452	0	0.7	"
    "8.505	5.715	2.27\n"
    "Rb	2.26	180	4.114	0.04	12	1.592	0	0.2	"
    "2.331	1.846	2.77\n"
    "Sr6+2	2.052	90	3.641	0.235	12	2.449	0	0.2	"
    "3.024	2.44	2.415\n"
    "Y_3+3	1.698	109.47	3.345	0.072	12	3.257	0	0.2	"
    "3.83	2.81	1.998\n"
    "Zr3+4	1.564	109.47	3.124	0.069	12	3.667	0	0.2	"
    "3.4	3.55	1.758\n"
    "Nb3+5	1.473	109.47	3.165	0.059	12	3.618	0	0.2	"
    "3.55	3.38	1.603\n"
    "Mo6+6	1.467	90	3.052	0.056	12	3.4	0	0.2	"
    "3.465	3.755	1.53\n"
    "Mo3+6	1.484	109.47	3.052	0.056	12	3.4	0	0.2	"
    "3.465	3.755	1.53\n"
    "Tc6+5	1.322	90	2.998	0.048	12	3.4	0	0.2	"
    "3.29	3.99	1.5\n"
    "Ru6+2	1.478	90	2.963	0.056	12	3.4	0	0.2	"
    "3.575	4.015	1.5\n"
    "Rh6+3	1.332	90	2.929	0.053	12	3.5	0	0.2	"
    "3.975	4.005	1.509\n"
    "Pd4+2	1.338	90	2.899	0.048	12	3.21	0	0.2	"
    "4.32	4	1.544\n"
    "Ag1+1	1.386	180	3.148	0.036	12	1.956	0	0.2	"
    "4.436	3.134	1.622\n"
    "Cd3+2	1.403	109.47	2.848	0.228	12	1.65	0	0.2	"
    "5.034	3.957	1.6\n"
    "In3+3	1.459	109.47	4.463	0.599	11	2.07	0	0.2	"
    "3.506	2.896	1.404\n"
    "Sn3	1.398	109.47	4.392	0.567	12	2.961	0.199	0.2	"
    "3.987	3.124	1.354\n"
    "Sb3+3	1.407	91.6	4.42	0.449	13	2.704	1.1	0.2	"
    "4.899	3.342	1.404\n"
    "Te3+2	1.386	90.25	4.47	0.398	14	2.882	0.3	0.2	"
    "5.816	3.526	1.38\n"
    "I_	1.382	180	4.5	0.339	15	2.65	0	0.2	"
    "6.822	3.762	1.333\n"
    "Xe4+4	1.267	90	4.404	0.332	12	0.556	0	0.2	"
    "7.595	4.975	2.459\n"
    "Cs	2.57	180	4.517	0.045	12	1.573	0	0.1	"
    "2.183	1.711	2.984\n"
    "Ba6+2	2.277	90	3.703	0.364	12	2.727	0	0.1	"
    "2.814	2.396	2.442\n"
    "La3+3	1.943	109.47	3.522	0.017	12	3.3	0	0.1	"
    "2.8355	2.7415	2.071\n"
    "Ce6+3	1.841	90	3.556	0.013	12	3.3	0	0.1	"
    "2.774	2.692	1.925\n"
    "Pr6+3	1.823	90	3.606	0.01	12	3.3	0	0.1	"
    "2.858	2.564	2.007\n"
    "Nd6+3	1.816	90	3.575	0.01	12	3.3	0	0.1	"
    "2.8685	2.6205	2.007\n"
    "Pm6+3	1.801	90	3.547	0.009	12	3.3	0	0.1	"
    "2.881	2.673	2\n"
    "Sm6+3	1.78	90	3.52	0.008	12	3.3	0	0.1	"
    "2.9115	2.7195	1.978\n"
    "Eu6+3	1.771	90	3.493	0.008	12	3.3	0	0.1	"
    "2.8785	2.7875	2.227\n"
    "Gd6+3	1.735	90	3.368	0.009	12	3.3	0	0.1	"
    "3.1665	2.9745	1.968\n"
    "Tb6+3	1.732	90	3.451	0.007	12	3.3	0	0.1	"
    "3.018	2.834	1.954\n"
    "Dy6+3	1.71	90	3.428	0.007	12	3.3	0	0.1	"
    "3.0555	2.8715	1.934\n"
    "Ho6+3	1.696	90	3.409	0.007	12	3.416	0	0.1	"
    "3.127	2.891	1.925\n"
    "Er6+3	1.673	90	3.391	0.007	12	3.3	0	0.1	"
    "3.1865	2.9145	1.915\n"
    "Tm6+3	1.66	90	3.374	0.006	12	3.3	0	0.1	"
    "3.2514	2.9329	2\n"
    "Yb6+3	1.637	90	3.355	0.228	12	2.618	0	0.1	"
    "3.2889	2.965	2.158\n"
    "Lu6+3	1.671	90	3.64	0.041	12	3.271	0	0.1	"
    "2.9629	2.4629	1.896\n"
    "Hf3+4	1.611	109.47	3.141	0.072	12	3.921	0	0.1	"
    "3.7	3.4	1.759\n"
    "Ta3+5	1.511	109.47	3.17	0.081	12	4.075	0	0.1	"
    "5.1	2.85	1.605\n"
    "W_6+6	1.392	90	3.069	0.067	12	3.7	0	0.1	"
    "4.63	3.31	1.538\n"
    "W_3+4	1.526	109.47	3.069	0.067	12	3.7	0	0.1	"
    "4.63	3.31	1.538\n"
    "W_3+6	1.38	109.47	3.069	0.067	12	3.7	0	0.1	"
    "4.63	3.31	1.538\n"
    "Re6+5	1.372	90	2.954	0.066	12	3.7	0	0.1	"
    "3.96	3.92	1.6\n"
    "Re3+7	1.314	109.47	2.954	0.066	12	3.7	0	0.1	"
    "3.96	3.92	1.6\n"
    "Os6+6	1.372	90	3.12	0.037	12	3.7	0	0.1	"
    "5.14	3.63	1.7\n"
    "Ir6+3	1.371	90	2.84	0.073	12	3.731	0	0.1	"
    "5	4	1.866\n"
    "Pt4+2	1.364	90	2.754	0.08	12	3.382	0	0.1	"
    "4.79	4.43	1.557\n"
    "Au4+3	1.262	90	3.293	0.039	12	2.625	0	0.1	"
    "4.894	2.586	1.618\n"
    "Hg1+2	1.34	180	2.705	0.385	12	1.75	0	0.1	"
    "6.27	4.16	1.6\n"
    "Tl3+3	1.518	120	4.347	0.68	11	2.068	0	0.1	"
    "3.2	2.9	1.53\n"
    "Pb3	1.459	109.47	4.297	0.663	12	2.846	0.1	0.1	"
    "3.9	3.53	1.444\n"
    "Bi3+3	1.512	90	4.37	0.518	13	2.47	1	0.1	"
    "4.69	3.74	1.514\n"
    "Po3+2	1.5	90	4.709	0.325	14	2.33	0.3	0.1	"
    "4.21	4.21	1.48\n"
    "At	1.545	180	4.75	0.284	15	2.24	0	0.1	"
    "4.75	4.75	1.47\n"
    "Rn4+4	1.42	90	4.765	0.248	16	0.583	0	0.1	"
    "5.37	5.37	2.2\n"
    "Fr	2.88	180	4.9	0.05	12	1.847	0	0	"
    "2	2	2.3\n"
    "Ra6+2	2.512	90	3.677	0.404	12	2.92	0	0	"
    "2.843	2.434	2.2\n"
    "Ac6+3	1.983	90	3.478	0.033	12	3.9	0	0	"
    "2.835	2.835	2.108\n"
    "Th6+4	1.721	90	3.396	0.026	12	4.202	0	0	"
    "3.175	2.905	2.018\n"
    "Pa6+4	1.711	90	3.424	0.022	12	3.9	0	0	"
    "2.985	2.905	1.8\n"
    "U_6+4	1.684	90	3.395	0.022	12	3.9	0	0	"
    "3.341	2.853	1.713\n"
    "Np6+4	1.666	90	3.424	0.019	12	3.9	0	0	"
    "3.549	2.717	1.8\n"
    "Pu6+4	1.657	90	3.424	0.016	12	3.9	0	0	"
    "3.243	2.819	1.84\n"
    "Am6+4	1.66	90	3.381	0.014	12	3.9	0	0	"
    "2.9895	3.0035	1.942\n"
    "Cm6+3	1.801	90	3.326	0.013	12	3.9	0	0	"
    "2.8315	3.1895	1.9\n"
    "Bk6+3	1.761	90	3.339	0.013	12	3.9	0	0	"
    "3.1935	3.0355	1.9\n"
    "Cf6+3	1.75	90	3.313	0.013	12	3.9	0	0	"
    "3.197	3.101	1.9\n"
    "Es6+3	1.724	90	3.299	0.012	12	3.9	0	0	"
    "3.333	3.089	1.9\n"
    "Fm6+3	1.712	90	3.286	0.012	12	3.9	0	0	"
    "3.4	3.1	1.9\n"
    "Md6+3	1.689	90	3.274	0.011	12	3.9	0	0	"
    "3.47	3.11	1.9\n"
    "No6+3	1.679	90	3.248	0.011	12	3.9	0	0	"
    "3.475	3.175	1.9\n"
    "Lw6+3	1.698	90	3.236	0.011	12	3.9	0	0	"
    "3.5	3.2	1.9\n";
}  // end of namespace UFF
}  // end of namespace ForceFields
