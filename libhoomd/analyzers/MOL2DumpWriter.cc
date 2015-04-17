/*
Highly Optimized Object-oriented Many-particle Dynamics -- Blue Edition
(HOOMD-blue) Open Source Software License Copyright 2009-2014 The Regents of
the University of Michigan All rights reserved.

HOOMD-blue may contain modifications ("Contributions") provided, and to which
copyright is held, by various Contributors who have granted The Regents of the
University of Michigan the right to modify and/or distribute such Contributions.

You may redistribute, use, and create derivate works of HOOMD-blue, in source
and binary forms, provided you abide by the following conditions:

* Redistributions of source code must retain the above copyright notice, this
list of conditions, and the following disclaimer both in the code and
prominently in any materials provided with the distribution.

* Redistributions in binary form must reproduce the above copyright notice, this
list of conditions, and the following disclaimer in the documentation and/or
other materials provided with the distribution.

* All publications and presentations based on HOOMD-blue, including any reports
or published results obtained, in whole or in part, with HOOMD-blue, will
acknowledge its use according to the terms posted at the time of submission on:
http://codeblue.umich.edu/hoomd-blue/citations.html

* Any electronic documents citing HOOMD-Blue will link to the HOOMD-Blue website:
http://codeblue.umich.edu/hoomd-blue/

* Apart from the above required attributions, neither the name of the copyright
holder nor the names of HOOMD-blue's contributors may be used to endorse or
promote products derived from this software without specific prior written
permission.

Disclaimer

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND/OR ANY
WARRANTIES THAT THIS SOFTWARE IS FREE OF INFRINGEMENT ARE DISCLAIMED.

IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Maintainer: joaander

/*! \file MOL2DumpWriter.cc
    \brief Defines the MOL2DumpWriter class
*/

#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4244 )
#endif

#include "MOL2DumpWriter.h"
#include "BondedGroupData.h"

#include <boost/python.hpp>
using namespace boost::python;

#include <iomanip>
#include <fstream>
#include <stdexcept>
#include <boost/shared_ptr.hpp>

using namespace std;

/*! \param sysdef SystemDefinition containing the ParticleData to dump
    \param fname_base The base file name to write the output to
*/
MOL2DumpWriter::MOL2DumpWriter(boost::shared_ptr<SystemDefinition> sysdef, std::string fname_base)
        : Analyzer(sysdef), m_base_fname(fname_base)
    {
    m_exec_conf->msg->notice(5) << "Constructing MOL2DumpWriter: " << fname_base << endl;
    }

MOL2DumpWriter::~MOL2DumpWriter()
    {
    m_exec_conf->msg->notice(5) << "Destroying PDBDumpWriter" << endl;
    }

/*! \param timestep Current time step of the simulation
    Writes a snapshot of the current state of the ParticleData to a mol2 file
*/
void MOL2DumpWriter::analyze(unsigned int timestep)
    {
    if (m_prof)
        m_prof->push("Dump MOL2");

    ostringstream full_fname;
    string filetype = ".mol2";

    // Generate a filename with the timestep padded to ten zeros
    full_fname << m_base_fname << "." << setfill('0') << setw(10) << timestep << filetype;
    writeFile(full_fname.str());

    if (m_prof)
        m_prof->pop();
    }

/*! \param fname File name to write
*/
void MOL2DumpWriter::writeFile(std::string fname)
    {
    // open the file for writing
    ofstream f(fname.c_str());

    if (!f.good())
        {
        m_exec_conf->msg->error() << "dump.mol2: Unable to open dump file for writing: " << fname << endl;
        throw runtime_error("Error writting mol2 dump file");
        }

    // acquire the particle data
    ArrayHandle<Scalar4> h_pos(m_pdata->getPositions(), access_location::host, access_mode::read);
    ArrayHandle<unsigned int> h_rtag(m_pdata->getRTags(), access_location::host, access_mode::read);

    // write the header
    f << "@<TRIPOS>MOLECULE" << "\n";
    f << "Generated by HOOMD" << "\n";
    int num_bonds = 1;
    boost::shared_ptr<BondData> bond_data = m_sysdef->getBondData();
    if (bond_data && bond_data->getN() > 0)
        num_bonds = bond_data->getN();

    f << m_pdata->getN() << " " << num_bonds << "\n";
    f << "NO_CHARGES" << "\n";

    f << "@<TRIPOS>ATOM" << "\n";
    for (unsigned int j = 0; j < m_pdata->getN(); j++)
        {
        // use the rtag data to output the particles in the order they were read in
        int i;
        i= h_rtag.data[j];

        // get the coordinates
        Scalar x = (h_pos.data[i].x);
        Scalar y = (h_pos.data[i].y);
        Scalar z = (h_pos.data[i].z);

        // get the type by name
        unsigned int type_id = __scalar_as_int(h_pos.data[i].w);
        string type_name = m_pdata->getNameByType(type_id);

        // this is intended to go to VMD, so limit the type name to 15 characters
        if (type_name.size() > 15)
            {
            m_exec_conf->msg->error() << "dump.mol2: Type name <" << type_name << "> too long: please limit to 15 characters" << endl;
            throw runtime_error("Error writting mol2 dump file");
            }

        f << j+1 << " " << type_name << " " << x << " " << y << " "<< z << " " << type_name << "\n";

        if (!f.good())
            {
            m_exec_conf->msg->error() << "dump.mol2: I/O error while writing MOL2 dump file" << endl;
            throw runtime_error("Error writting mol2 dump file");
            }
        }

    f << "@<TRIPOS>BOND" << "\n";
    if (bond_data && bond_data->getN() > 0)
        {
        for (unsigned int i = 0; i < bond_data->getN(); i++)
            {
            BondData::members_t b = bond_data->getMembersByIndex(i);
            f << i+1 << " " << b.tag[0]+1 << " " << b.tag[1]+1 << " 1" << "\n";
            }
        }
    else
        {
        // write a dummy bond since VMD doesn't like loading mol2 files without bonds
        f << "1 1 2 1" << "\n";
        }

    if (!f.good())
        {
        m_exec_conf->msg->error() << "dump.mol2: I/O error while writing file" << endl;
        throw runtime_error("Error writting mol2 dump file");
        }

    f.close();
    }

void export_MOL2DumpWriter()
    {
    class_<MOL2DumpWriter, boost::shared_ptr<MOL2DumpWriter>, bases<Analyzer>, boost::noncopyable>
    ("MOL2DumpWriter", init< boost::shared_ptr<SystemDefinition>, std::string >())
    .def("writeFile", &MOL2DumpWriter::writeFile)
    ;

    }

#ifdef WIN32
#pragma warning( pop )
#endif
