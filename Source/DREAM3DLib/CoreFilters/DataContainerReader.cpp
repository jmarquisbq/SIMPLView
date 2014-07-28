/* ============================================================================
 * Copyright (c) 2012 Michael A. Jackson (BlueQuartz Software)
 * Copyright (c) 2012 Dr. Michael A. Groeber (US Air Force Research Laboratories)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of Michael A. Groeber, Michael A. Jackson, the US Air Force,
 * BlueQuartz Software nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  This code was written under United States Air Force Contract number
 *                           FA8650-07-D-5800
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "DataContainerReader.h"

#include <QtCore/QFileInfo>

#include "H5Support/QH5Utilities.h"
#include "H5Support/QH5Lite.h"
#include "H5Support/HDF5ScopedFileSentinel.h"

#include "DREAM3DLib/FilterParameters/H5FilterParametersReader.h"
#include "DREAM3DLib/Common/FilterManager.h"


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
DataContainerReader::DataContainerReader() :
  AbstractFilter(),
  m_InputFile(""),
  m_OverwriteExistingDataContainers(false)
{
  m_PipelineFromFile = FilterPipeline::New();
  setupFilterParameters();
  m_DataContainerArrayProxy.isValid = false;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
DataContainerReader::~DataContainerReader()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DataContainerReader::setupFilterParameters()
{
  FilterParameterVector parameters;
  parameters.push_back(FileSystemFilterParameter::New("Input File", "InputFile", FilterParameterWidgetType::InputFileWidget, getInputFile(), false, "", "*.dream3d"));
  parameters.push_back(FilterParameter::New("Overwrite Existing DataContainers", "OverwriteExistingDataContainers", FilterParameterWidgetType::BooleanWidget, getOverwriteExistingDataContainers(), false));
  {
    DataContainerArrayProxyFilterParameter::Pointer parameter = DataContainerArrayProxyFilterParameter::New();
    parameter->setHumanLabel("Selected Arrays");
    parameter->setPropertyName("DataContainerArrayProxy");
    parameter->setWidgetType(FilterParameterWidgetType::DataContainerArrayProxyWidget);
    parameters.push_back(parameter);
  }
  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DataContainerReader::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  setInputFile(reader->readString("InputFile", getInputFile() ) );
  setOverwriteExistingDataContainers(reader->readValue("OverwriteExistingDataContainers", getOverwriteExistingDataContainers() ) );
  setDataContainerArrayProxy(reader->readDataContainerArrayProxy("DataContainerArrayProxy", getDataContainerArrayProxy() ) );
  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int DataContainerReader::writeFilterParameters(AbstractFilterParametersWriter* writer, int index)
{
  index = writeExistingPipelineToFile(writer, index);

  writer->openFilterGroup(this, index);
  DREAM3D_FILTER_WRITE_PARAMETER(InputFile)
  DREAM3D_FILTER_WRITE_PARAMETER(OverwriteExistingDataContainers)
  DataContainerArrayProxy dcaProxy = getDataContainerArrayProxy(); // This line makes a COPY of the DataContainerArrayProxy that is stored in the current instance
  writer->writeValue("DataContainerArrayProxy", dcaProxy );


  writer->closeFilterGroup();
  return ++index; // we want to return the index after the one we just wrote to
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DataContainerReader::dataCheck()
{
  QString ss;
  QFileInfo fi(getInputFile());
  if (getInputFile().isEmpty() == true)
  {
    ss = QObject::tr("%1 needs the Input File Set and it was not.").arg(ClassName());
    setErrorCondition(-387);
    notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
  }
  else if (fi.exists() == false)
  {
    ss = QObject::tr("The input file does not exist.");
    setErrorCondition(-388);
    notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DataContainerReader::preflight()
{
  setInPreflight(true);
  // Get the current structure of the file. We want ALL of the structure. so set it into a Proxy
  DataContainerArrayProxy proxy = readDataContainerArrayStructure();

  // to the read here because this will populate the DataContainerArray with our DataContainer
  dataCheck();
  if(getErrorCondition() >= 0)
  {
    readData(true, proxy); // Read using the local Proxy object which will force the read to create a full structure DataContainer
  }

  // Annouce we are about to preflight
  // The GUI will pick up the structure
  emit preflightAboutToExecute();

  // The Gui sends down any changes to the Proxy (which for preflight we don't care about)
  emit updateFilterParameters(this);

  // The GUI needs to send down the selections that were made by the user and we need to update
  // DataContainerArray->DataContainer object so the rest of the pipeline has the proper information
  emit preflightExecuted(); // Done executing

  // Get the latest DataContainer Array from the filter
  DataContainerArray::Pointer dca = getDataContainerArray();
  // Prune out those child items that are NOT selected in the model
  m_DataContainerArrayProxy.removeSelectionsFromDataContainerArray(dca.get());

}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DataContainerReader::execute()
{
  dataCheck();
  if(getErrorCondition() >= 0)
  {
    readData(false, m_DataContainerArrayProxy); // This time do the read with the users selections for real
  }
  notifyStatusMessage(getHumanLabel(), "Complete");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void DataContainerReader::readData(bool preflight, DataContainerArrayProxy& proxy)
{
  setErrorCondition(0);
  QString ss;
  int32_t err = 0;
  QString m_FileVersion;
  float fVersion = 0.0f;
  bool check = false;

  //  qDebug() << "DataContainerReader::readData() " << m_InputFile;

  // Read the Meta Data and Array names from the file
  hid_t fileId = QH5Utilities::openFile(m_InputFile, true); // Open the file Read Only
  if(fileId < 0)
  {
    ss = QObject::tr(": Error opening input file '%1'").arg(ClassName());
    setErrorCondition(-150);
    notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
    return;
  }
  // This will make sure if we return early from this method that the HDF5 File is properly closed.
  HDF5ScopedFileSentinel scopedFileSentinel(&fileId, true);

  //Check to see if version of .dream3d file is prior to new data container names
  err = QH5Lite::readStringAttribute(fileId, "/", DREAM3D::HDF5::FileVersionName, m_FileVersion);
  fVersion = m_FileVersion.toFloat(&check);
  if(fVersion < 5.0 || err < 0)
  {
    QH5Utilities::closeFile(fileId);
    fileId = QH5Utilities::openFile(m_InputFile, false); // Re-Open the file as Read/Write
    err = H5Lmove(fileId, "VoxelDataContainer", fileId, DREAM3D::Defaults::VolumeDataContainerName.toLatin1().data(), H5P_DEFAULT, H5P_DEFAULT);
    err = H5Lmove(fileId, "SurfaceMeshDataContainer", fileId, DREAM3D::Defaults::SurfaceDataContainerName.toLatin1().data(), H5P_DEFAULT, H5P_DEFAULT);
    err = QH5Lite::writeStringAttribute(fileId, "/", DREAM3D::HDF5::FileVersionName, "5.0");
    QH5Utilities::closeFile(fileId);
    fileId = QH5Utilities::openFile(m_InputFile, true); // Re-Open the file as Read Only
  }
  if(fVersion < 6.0)
  {
    ss = QObject::tr(": File unable to be read - file structure older than 6.0 '%1'").arg(ClassName());
    setErrorCondition(-250);
    notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
    return;
  }
  hid_t dcaGid = H5Gopen(fileId, DREAM3D::StringConstants::DataContainerGroupName.toLatin1().data(), 0);
  if(dcaGid < 0)
  {
    setErrorCondition(-1923123);
    QString ss = QObject::tr("An error occurred attempting to open the HDF5 Group '%1'").arg(DREAM3D::StringConstants::DataContainerGroupName);
    notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
    return;
  }


  scopedFileSentinel.addGroupId(&dcaGid);

  DataContainerArray::Pointer dca = getDataContainerArray();
  err = dca->readDataContainersFromHDF5(preflight, dcaGid, proxy, this);

  if(err < 0)
  {
    setErrorCondition(err);
    QString ss = QObject::tr("An error occurred trying to read the DataContainers from the file '%1'").arg(getInputFile());
    notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
  }

  err = H5Gclose(dcaGid);
  dcaGid = -1;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
DataContainerArrayProxy DataContainerReader::readDataContainerArrayStructure()
{
  DataContainerArrayProxy proxy(false);
  if (getInputFile().isEmpty() == true)
  {
    QString ss = QObject::tr("DREAM3D File Path is empty.");
    setErrorCondition(-70);
    notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
    return proxy;
  }
  herr_t err = 0;
  hid_t fileId = QH5Utilities::openFile(getInputFile(), true);
  if(fileId < 0)
  {
    QString ss = QObject::tr("Error opening DREAM3D file location at %1").arg(getInputFile());
    setErrorCondition(-71);
    notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
    return proxy;
  }
  HDF5ScopedFileSentinel sentinel(&fileId, false); // Make sure the file gets closed automatically if we return early

  //Check the DREAM3D File Version to make sure we are reading the proper version
  QString d3dVersion;
  err = QH5Lite::readStringAttribute(fileId, "/", DREAM3D::HDF5::DREAM3DVersion, d3dVersion);
  if (err < 0)
  {
    QString ss = QObject::tr("HDF5 Attribute '%1' was not found on the HDF5 root node and this is required.").arg(DREAM3D::HDF5::DREAM3DVersion);
    setErrorCondition(-72);
    notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
    return proxy;
  }
  //  else {
  //    std::cout << DREAM3D::HDF5::DREAM3DVersion.toStdString() << ":" << d3dVersion.toStdString() << std::endl;
  //  }

  QString fileVersion;
  err = QH5Lite::readStringAttribute(fileId, "/", DREAM3D::HDF5::FileVersionName, fileVersion);
  if (err < 0)
  {
    //std::cout << "Attribute '" << DREAM3D::HDF5::FileVersionName.toStdString() << " was not found" << std::endl;
    QString ss = QObject::tr("HDF5 Attribute '%1' was not found on the HDF5 root node and this is required.").arg(DREAM3D::HDF5::FileVersionName);
    setErrorCondition(-73);
    notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
    return proxy;
  }
  //  else {
  //    std::cout << DREAM3D::HDF5::FileVersionName.toStdString() << ":" << fileVersion.toStdString() << std::endl;
  //  }

  hid_t dcArrayGroupId = H5Gopen(fileId, DREAM3D::StringConstants::DataContainerGroupName.toAscii().constData(), H5P_DEFAULT);
  if (dcArrayGroupId < 0)
  {
    QString ss = QObject::tr("Error opening HDF5 Group '%1' ").arg(DREAM3D::StringConstants::DataContainerGroupName);
    setErrorCondition(-74);
    notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
    return proxy;
  }
  sentinel.addGroupId(&dcArrayGroupId);


  QString h5InternalPath = QString("/") + DREAM3D::StringConstants::DataContainerGroupName;

  // Read the entire structure of the file into the proxy
  DataContainer::ReadDataContainerStructure(dcArrayGroupId, proxy, h5InternalPath);
  proxy.isValid = true; // Make the DataContainerArrayProxy valid

  return proxy;
}


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int DataContainerReader::readExistingPipelineFromFile(hid_t fileId)
{
  int err = 0;
  m_PipelineFromFile->clear();

  H5FilterParametersReader::Pointer reader = H5FilterParametersReader::New();

  // HDF5: Open the "Pipeline" Group
  hid_t pipelineGroupId = H5Gopen(fileId, DREAM3D::StringConstants::PipelineGroupName.toLatin1().data(), H5P_DEFAULT);
  reader->setPipelineGroupId(pipelineGroupId);

  // Use QH5Lite to ask how many "groups" are in the "Pipeline Group"
  QList<QString> groupList;
  err = QH5Utilities::getGroupObjects(pipelineGroupId, H5Utilities::H5Support_GROUP, groupList);

  // Loop over the items getting the "ClassName" attribute from each group
  QString classNameStr = "";
  for (int i = 0; i < groupList.size(); i++)
  {
    QString ss = QString::number(i, 10);

    err = QH5Lite::readStringAttribute(pipelineGroupId, ss, "ClassName", classNameStr);
    // Instantiate a new filter using the FilterFactory based on the value of the className attribute
    FilterManager* fm = FilterManager::Instance();
    IFilterFactory::Pointer ff = fm->getFactoryForFilter(classNameStr);
    if (NULL != ff.get())
    {
      AbstractFilter::Pointer filter = ff->create();
      if(NULL != filter.get())
      {
        // Read the parameters
        filter->readFilterParameters( reader.get(), i);

        // Add filter to m_PipelineFromFile
        m_PipelineFromFile->pushBack(filter);
      }
    }
  }
  err = H5Gclose(pipelineGroupId);
  return err;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int DataContainerReader::writeExistingPipelineToFile(AbstractFilterParametersWriter* writer, int index)
{
  FilterPipeline::FilterContainerType container = m_PipelineFromFile->getFilterContainer();

  for(FilterPipeline::FilterContainerType::iterator iter = container.begin(); iter != container.end(); ++iter)
  {
    index = (*iter)->writeFilterParameters(writer, index);
  }
  return index;
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer DataContainerReader::newFilterInstance(bool copyFilterParameters)
{
  /*
  * InputFile
  * OverwriteExistingDataContainers
  * DataContainerArrayProxy
  */
  DataContainerReader::Pointer filter = DataContainerReader::New();
  if(true == copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
#if 0
    filter->setInputFile(getInputFile());
    filter->setOverwriteExistingDataContainers(getOverwriteExistingDataContainers());
    filter->setDataContainerArrayProxy(getDataContainerArrayProxy());
#endif
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
const QString DataContainerReader::getCompiledLibraryName()
{ return Core::CoreBaseName;  }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
const QString DataContainerReader::getGroupName()
{ return DREAM3D::FilterGroups::IOFilters; }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
const QString DataContainerReader::getSubGroupName()
{ return DREAM3D::FilterSubGroups::InputFilters; }


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
const QString DataContainerReader::getHumanLabel()
{ return "Read DREAM3D Data File"; }

