///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2007, 2010 Michael A. Jackson for BlueQuartz Software
//  All rights reserved.
//  BSD License: http://www.opensource.org/licenses/bsd-license.html
//
//  This code was written under United States Air Force Contract number
//                           FA8650-04-C-5229
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#ifdef _MSC_VER
#include <cstdlib>


/*
 "It's a known, long-standing bug in the compiler system's headers.  For
 some reason the manufacturer, in its infinite wisdom, chose to #define
 macros min() and max() in violation of the upper-case convention and so
 break any legitimate functions with those names, including those in the
 standard C++ library."
 */

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define WINDOWS_LARGE_FILE_SUPPORT
#if _MSC_VER < 1400
#define snprintf _snprintf
#else
#define snprintf sprintf_s
#endif

#endif


//-- C++ includes
#include <iostream>

#include <boost/assert.hpp>

#include "DREAM3DLib/Utilities/UnitTestSupport.hpp"
#include "TestFileLocations.h"

#include "DREAM3DLib/CoreFilters/RemoveArrays.h"
#include "DREAM3DLib/CoreFilters/RenameDataContainer.h"
#include "DREAM3DLib/CoreFilters/RenameAttributeMatrix.h"
#include "DREAM3DLib/CoreFilters/RenameAttributeArray.h"
#include "DREAM3DLib/CoreFilters/MoveData.h"
#include "DREAM3DLib/CoreFilters/CopyDataContainer.h"
#include "DREAM3DLib/CoreFilters/CopyAttributeMatrix.h"
#include "DREAM3DLib/CoreFilters/CopyAttributeArray.h"

static const int k_MoveAttributeMatrix = 0;
static const int k_MoveDataArray = 1;

enum ErrorCodes
{
    DC_SELECTED_NAME_EMPTY = -11000,
    DC_NEW_NAME_EMPTY = -11001,
    DC_SELECTED_NOT_FOUND = -11002,
    DCA_NOT_FOUND = -11003,
    AM_NEW_NAME_EMPTY = -11004,
    AM_SELECTED_PATH_EMPTY = -11005,
    RENAME_ATTEMPT_FAILED = -11006,
    DC_NOT_FOUND = -11007,
    AM_NOT_FOUND = -11008,
    AA_NEW_NAME_EMPTY = -11009,
    AA_SELECTED_PATH_EMPTY = -11010,
    DC_DEST_NOT_FOUND = -11011,
    DC_SRC_NOT_FOUND = -11012,
    AM_SRC_NOT_FOUND = -11013,
    AA_NOT_FOUND = -11014,
    TUPLES_NOT_MATCH = -11019
};



// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
DataContainerArray::Pointer createDataContainerArray()
{
    DataContainerArray::Pointer dca = DataContainerArray::New();
    VolumeDataContainer::Pointer dc1 = VolumeDataContainer::New("DataContainer1");
    VolumeDataContainer::Pointer dc2 = VolumeDataContainer::New("DataContainer2");
    AttributeMatrix::Pointer am1 = AttributeMatrix::New(QVector<size_t>(3,2), "AttributeMatrix1", 0);
    AttributeMatrix::Pointer am2 = AttributeMatrix::New(QVector<size_t>(7,2), "AttributeMatrix2", 0);
    AttributeMatrix::Pointer am3 = AttributeMatrix::New(QVector<size_t>(4,3), "AttributeMatrix3", 0);
    AttributeMatrix::Pointer am4 = AttributeMatrix::New(QVector<size_t>(7,2), "AttributeMatrix4", 0);
    IDataArray::Pointer da1 = DataArray<size_t>::CreateArray(8, "DataArray1");
    IDataArray::Pointer da2 = DataArray<size_t>::CreateArray(128, "DataArray2");
    IDataArray::Pointer da3 = DataArray<size_t>::CreateArray(128, "DataArray3");
    IDataArray::Pointer da4 = DataArray<size_t>::CreateArray(81, "DataArray4");
    IDataArray::Pointer da5 = DataArray<size_t>::CreateArray(81, "DataArray5");
    
    am1->addAttributeArray("DataArray1", da1);
    am2->addAttributeArray("DataArray2", da2);
    am2->addAttributeArray("DataArray3", da3);
    am3->addAttributeArray("DataArray4", da4);
    am3->addAttributeArray("DataArray5", da5);
    
    dc1->addAttributeMatrix("AttributeMatrix1", am1);
    dc1->addAttributeMatrix("AttributeMatrix2", am2);
    dc2->addAttributeMatrix("AttributeMatrix3", am3);
    dc2->addAttributeMatrix("AttributeMatrix4", am4);
    
    dca->addDataContainer(dc1);
    dca->addDataContainer(dc2);
    
    return dca;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void MoveDataTest()
{
    DataContainerArray::Pointer dca = createDataContainerArray();
    
    MoveData::Pointer moveDataPtr = MoveData::New();
    moveDataPtr->setDataContainerArray(dca);
    moveDataPtr->setWhatToMove(k_MoveAttributeMatrix);
    
    // "Destination Data Container Does Not Exist" Test
    moveDataPtr->setDataContainerDestination("ThisDataContainerShouldNotExist");
    DataArrayPath amSource("DataContainer1", "AttributeMatrix1", "");
    moveDataPtr->setAttributeMatrixSource(amSource);
    moveDataPtr->execute();
    DREAM3D_REQUIRE_EQUAL(moveDataPtr->getErrorCondition(), DC_DEST_NOT_FOUND)
    
    // "Source Data Container Does Not Exist" Test
    moveDataPtr->setDataContainerDestination("DataContainer2");
    moveDataPtr->setAttributeMatrixSource(DataArrayPath("ThisDataContainerShouldNotExist", "AttributeMatrix1", ""));
    moveDataPtr->execute();
    DREAM3D_REQUIRE_EQUAL(moveDataPtr->getErrorCondition(), DC_SRC_NOT_FOUND)
    
    // "Source Attribute Matrix Does Not Exist" Test
    moveDataPtr->setDataContainerDestination("DataContainer2");
    moveDataPtr->setAttributeMatrixSource(DataArrayPath("DataContainer1", "ThisAttributeMatrixShouldNotExist", ""));
    moveDataPtr->execute();
    DREAM3D_REQUIRE_EQUAL(moveDataPtr->getErrorCondition(), AM_SRC_NOT_FOUND)
    
    // "Move Attribute Matrix" Verification Test
    moveDataPtr->setAttributeMatrixSource(DataArrayPath("DataContainer1", "AttributeMatrix1", ""));
    moveDataPtr->setDataContainerDestination("DataContainer2");
    moveDataPtr->execute();
    DREAM3D_REQUIRE_EQUAL(moveDataPtr->getErrorCondition(), 0)
    
    moveDataPtr->setWhatToMove(k_MoveDataArray);
    
    // "Creation of Data Array Pointer" Test
    moveDataPtr->setDataArraySource(DataArrayPath("DataContainer1", "AttributeMatrix1", "ThisShouldNotExist"));
    moveDataPtr->execute();
    DREAM3D_REQUIRED(moveDataPtr->getErrorCondition(), <, 0)
    
    moveDataPtr->setDataArraySource(DataArrayPath("DataContainer1", "ThisShouldNotExist", "DataArray1"));
    moveDataPtr->execute();
    DREAM3D_REQUIRED(moveDataPtr->getErrorCondition(), <, 0)
    
    moveDataPtr->setDataArraySource(DataArrayPath("ThisShouldNotExist", "AttributeMatrix1", "DataArray1"));
    moveDataPtr->execute();
    DREAM3D_REQUIRED(moveDataPtr->getErrorCondition(), <, 0)
    
    // "Creation of Attribute Matrix Pointer" Test
    moveDataPtr->setAttributeMatrixDestination(DataArrayPath("DataContainer2", "ThisShouldNotExist", ""));
    moveDataPtr->execute();
    DREAM3D_REQUIRED(moveDataPtr->getErrorCondition(), <, 0)
    
    moveDataPtr->setAttributeMatrixDestination(DataArrayPath("ThisShouldNotExist", "AttributeMatrix3", ""));
    moveDataPtr->execute();
    DREAM3D_REQUIRED(moveDataPtr->getErrorCondition(), <, 0)
    
    // "NumTuples Comparison" Test
    moveDataPtr->setAttributeMatrixDestination(DataArrayPath("DataContainer2", "AttributeMatrix3", ""));
    moveDataPtr->setDataArraySource(DataArrayPath("DataContainer1", "AttributeMatrix2", "DataArray3"));
    moveDataPtr->execute();
    DREAM3D_REQUIRE_EQUAL(moveDataPtr->getErrorCondition(), TUPLES_NOT_MATCH)
    
    // "Move Data Array" Verification Test
    moveDataPtr->setAttributeMatrixDestination(DataArrayPath("DataContainer2", "AttributeMatrix4", ""));
    moveDataPtr->setDataArraySource(DataArrayPath("DataContainer1", "AttributeMatrix2", "DataArray3"));
    moveDataPtr->execute();
    DREAM3D_REQUIRE_EQUAL(moveDataPtr->getErrorCondition(), 0)
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void CopyDataTest()
{
    /***** Copy Data Container *****/
    
    // "DataContainerArray Not Found" Test
    CopyDataContainer::Pointer copyDataContainerPtr = CopyDataContainer::New();
    
    DataContainerArray::Pointer dca_not_found = DataContainerArray::NullPointer();
    copyDataContainerPtr->setDataContainerArray(dca_not_found);
    copyDataContainerPtr->setSelectedDataContainerName("DataContainer1");
    copyDataContainerPtr->setNewDataContainerName("DataContainer3");
    copyDataContainerPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyDataContainerPtr->getErrorCondition(), DCA_NOT_FOUND)
    
    DataContainerArray::Pointer dca = createDataContainerArray();
    copyDataContainerPtr->setDataContainerArray(dca);
    
    // "Selected Data Container Name is Empty" Test
    copyDataContainerPtr->setSelectedDataContainerName("");
    copyDataContainerPtr->setNewDataContainerName("DataContainer3");
    copyDataContainerPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyDataContainerPtr->getErrorCondition(), DC_SELECTED_NAME_EMPTY)
    
    // "New Data Container Name is Empty" Test
    copyDataContainerPtr->setSelectedDataContainerName("DataContainer1");
    copyDataContainerPtr->setNewDataContainerName("");
    copyDataContainerPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyDataContainerPtr->getErrorCondition(), DC_NEW_NAME_EMPTY)
    
    // "Selected Data Container Does Not Exist" Test
    copyDataContainerPtr->setSelectedDataContainerName("ThisShouldNotExist");
    copyDataContainerPtr->setNewDataContainerName("DataContainer3");
    copyDataContainerPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyDataContainerPtr->getErrorCondition(), DC_SELECTED_NOT_FOUND)
    
    // Copy Data Container Verification Test
    copyDataContainerPtr->setSelectedDataContainerName("DataContainer1");
    copyDataContainerPtr->setNewDataContainerName("DataContainer3");
    copyDataContainerPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyDataContainerPtr->getErrorCondition(), 0)
    
    /***** Copy Attribute Matrix *****/
    CopyAttributeMatrix::Pointer copyAttrMatPtr = CopyAttributeMatrix::New();
    dca = createDataContainerArray();
    copyAttrMatPtr->setDataContainerArray(dca);
    
    // "New Attribute Matrix Name is Empty" Test
    copyAttrMatPtr->setSelectedAttributeMatrixPath(DataArrayPath("DataContainer1", "AttributeMatrix1", ""));
    copyAttrMatPtr->setNewAttributeMatrix("");
    copyAttrMatPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyAttrMatPtr->getErrorCondition(), AM_NEW_NAME_EMPTY)
    
    // "Selected Attribute Matrix Path is Empty" Test
    copyAttrMatPtr->setSelectedAttributeMatrixPath(DataArrayPath("", "", ""));
    copyAttrMatPtr->setNewAttributeMatrix("Foo");
    copyAttrMatPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyAttrMatPtr->getErrorCondition(), AM_SELECTED_PATH_EMPTY)

    // "Data Container Not Found" Test
    copyAttrMatPtr->setSelectedAttributeMatrixPath(DataArrayPath("ThisShouldNotBeFound", "AttributeMatrix1", ""));
    copyAttrMatPtr->setNewAttributeMatrix("AttributeMatrix10");
    copyAttrMatPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyAttrMatPtr->getErrorCondition(), DC_NOT_FOUND)
    
    // "Attribute Matrix Not Found" Test
    copyAttrMatPtr->setSelectedAttributeMatrixPath(DataArrayPath("DataContainer1", "ThisShouldNotBeFound", ""));
    copyAttrMatPtr->setNewAttributeMatrix("AttributeMatrix10");
    copyAttrMatPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyAttrMatPtr->getErrorCondition(), AM_NOT_FOUND)
    
    // "Copy Attribute Matrix" Verification Test
    copyAttrMatPtr->setSelectedAttributeMatrixPath(DataArrayPath("DataContainer1", "AttributeMatrix1", ""));
    copyAttrMatPtr->setNewAttributeMatrix("AttributeMatrix10");
    copyAttrMatPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyAttrMatPtr->getErrorCondition(), 0)
    
    /***** Copy Attribute Array *****/
    CopyAttributeArray::Pointer copyAttrArrayPtr = CopyAttributeArray::New();
    dca = createDataContainerArray();
    copyAttrArrayPtr->setDataContainerArray(dca);
    
    // "New Data Array Name Empty" Test
    copyAttrArrayPtr->setSelectedArrayPath(DataArrayPath("DataContainer1", "AttributeMatrix1", "DataArray1"));
    copyAttrArrayPtr->setNewArrayName("");
    copyAttrArrayPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyAttrArrayPtr->getErrorCondition(), AA_NEW_NAME_EMPTY)
    
    // "Data Array Path Empty" Test
    copyAttrArrayPtr->setSelectedArrayPath(DataArrayPath("", "", ""));
    copyAttrArrayPtr->setNewArrayName("NewDataArrayName");
    copyAttrArrayPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyAttrArrayPtr->getErrorCondition(), AA_SELECTED_PATH_EMPTY)
    
    // "Data Container Not Found" Test
    copyAttrArrayPtr->setSelectedArrayPath(DataArrayPath("ThisShouldNotExist", "AttributeMatrix1", "DataArray1"));
    copyAttrArrayPtr->setNewArrayName("NewDataArrayName");
    copyAttrArrayPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyAttrArrayPtr->getErrorCondition(), DC_NOT_FOUND)
    
    // "Attribute Matrix Not Found" Test
    copyAttrArrayPtr->setSelectedArrayPath(DataArrayPath("DataContainer1", "ThisShouldNotExist", "DataArray1"));
    copyAttrArrayPtr->setNewArrayName("NewDataArrayName");
    copyAttrArrayPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyAttrArrayPtr->getErrorCondition(), AM_NOT_FOUND)
    
    // "Data Array Not Found" Test
    copyAttrArrayPtr->setSelectedArrayPath(DataArrayPath("DataContainer1", "AttributeMatrix1", "ThisShouldNotExist"));
    copyAttrArrayPtr->setNewArrayName("NewDataArrayName");
    copyAttrArrayPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyAttrArrayPtr->getErrorCondition(), AA_NOT_FOUND)
    
    // "Copy Data Array" Verification Test
    copyAttrArrayPtr->setSelectedArrayPath(DataArrayPath("DataContainer1", "AttributeMatrix1", "DataArray1"));
    copyAttrArrayPtr->setNewArrayName("NewDataArrayName");
    copyAttrArrayPtr->execute();
    DREAM3D_REQUIRE_EQUAL(copyAttrArrayPtr->getErrorCondition(), 0)
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RenameDataTest()
{
    /***** Rename Data Container *****/
    RenameDataContainer::Pointer renameDataContainerPtr = RenameDataContainer::New();
    
    DataContainerArray::Pointer dca_not_found = DataContainerArray::NullPointer();
    renameDataContainerPtr->setDataContainerArray(dca_not_found);
    
    // "DataContainerArray Is Null" Test
    renameDataContainerPtr->setNewDataContainerName("IShouldBeAbleToRenameThis");
    renameDataContainerPtr->setSelectedDataContainerName("DataContainer1");
    renameDataContainerPtr->execute();
    DREAM3D_REQUIRE_EQUAL(renameDataContainerPtr->getErrorCondition(), DCA_NOT_FOUND)
    
    DataContainerArray::Pointer dca = createDataContainerArray();
    renameDataContainerPtr->setDataContainerArray(dca);
    
    // "New Data Container Name Is Empty" Test
    renameDataContainerPtr->setNewDataContainerName("");
    renameDataContainerPtr->setSelectedDataContainerName("DataContainer1");
    renameDataContainerPtr->execute();
    DREAM3D_REQUIRE_EQUAL(renameDataContainerPtr->getErrorCondition(), DC_NEW_NAME_EMPTY)
    
    // "Rename Attempt Failed" Test
    renameDataContainerPtr->setNewDataContainerName("DataContainer2");
    renameDataContainerPtr->setSelectedDataContainerName("DataContainer1");
    renameDataContainerPtr->execute();
    DREAM3D_REQUIRE_EQUAL(renameDataContainerPtr->getErrorCondition(), RENAME_ATTEMPT_FAILED)
    
    // "Selected Data Container Does Not Exist" Test
    renameDataContainerPtr->setNewDataContainerName("IShouldBeAbleToRenameThis");
    renameDataContainerPtr->setSelectedDataContainerName("ThisNameShouldNotExist");
    renameDataContainerPtr->execute();
    DREAM3D_REQUIRE_EQUAL(renameDataContainerPtr->getErrorCondition(), RENAME_ATTEMPT_FAILED)
    
    // "Rename Data Container" Verification Test
    renameDataContainerPtr->setNewDataContainerName("IShouldBeAbleToRenameThis");
    renameDataContainerPtr->setSelectedDataContainerName("DataContainer1");
    renameDataContainerPtr->execute();
    DREAM3D_REQUIRE_EQUAL(renameDataContainerPtr->getErrorCondition(), 0)
    
    /***** Rename Attribute Matrix *****/
    RenameAttributeMatrix::Pointer renameAttrMatPtr = RenameAttributeMatrix::New();
    
    renameAttrMatPtr->setDataContainerArray(dca_not_found);
    //renameAttrMatPtr->setSelectedAttributeMatrixPath(<#DataArrayPath value#>);
    //renameAttrMatPtr->setNewAttributeMatrix(<#QString value#>);
    renameAttrMatPtr->execute();
    DREAM3D_REQUIRE_EQUAL(renameAttrMatPtr->getErrorCondition(), DCA_NOT_FOUND)
    
    DataContainerArray::Pointer dca2 = createDataContainerArray();
    renameAttrMatPtr->setDataContainerArray(dca2);
    
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RemoveDataTest()
{
    
}

// -----------------------------------------------------------------------------
//  Use unit test framework
// -----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    int err = EXIT_SUCCESS;
    
    DREAM3D_REGISTER_TEST( MoveDataTest() )
    DREAM3D_REGISTER_TEST( CopyDataTest() )
    //DREAM3D_REGISTER_TEST( RenameDataTest() )
    //DREAM3D_REGISTER_TEST( RemoveDataTest() )
    
    PRINT_TEST_SUMMARY();
    
    return err;
}


