/*
 For more information, please see: http://software.sci.utah.edu
 
 The MIT License
 
 Copyright (c) 2011 Scientific Computing and Imaging Institute,
 University of Utah.
 
 
 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
 */

// Specific includes for reading large datafiles
// NOTE: We needed to special case Windows as VS 2008 has an improper 64bit seek function in its
// STL implementation.
#ifdef _WIN32
#include <windows.h>
#else
#include <fstream>
#endif

#include <iostream>
// Core includes
#include <Core/DataBlock/StdDataBlock.h>

// Application includes
#include <Application/Layer/DataLayer.h> 
#include <Application/LayerIO/MRCLayerImporter.h>
#include <Application/LayerIO/LayerIO.h>

#include <mrcheader.h>
#include <MRCReader.h>

SEG3D_REGISTER_IMPORTER( Seg3D, MRCLayerImporter );

namespace Seg3D
{
  
  //////////////////////////////////////////////////////////////////////////
  // Class MRCLayerImporterPrivate
  //////////////////////////////////////////////////////////////////////////
  
  class MRCLayerImporterPrivate : public boost::noncopyable
  {
  public:
    MRCLayerImporterPrivate() : importer_( 0 ),
		  data_type_( Core::DataType::UNKNOWN_E ),
		  read_header_( false ),
		  read_data_( false )
    {
    }

    // Pointer back to the main class
    MRCLayerImporter* importer_;

  public:
    // Datablock that was extracted
    Core::DataBlockHandle data_block_;

    // Grid transform that was extracted
    Core::GridTransform	grid_transform_;

    // Type of the pixels in the file
    Core::DataType data_type_;

    // Meta data (generated by read_data)
    LayerMetaData meta_data_;

    // MRC header and reader classes
    MRC2000IO::MRCHeader header_;
    MRC2000IO::MRCReader mrcreader_;
    
    // Whether the header has been read
    bool read_header_;
    
    // Whether the data has been read
    bool read_data_;

  public:
    // READ_HEADER
    // Read the header of the file
    bool read_header();
    
    // READ_DATA
    // Read the data from the file
    bool read_data();
  };
  
  bool MRCLayerImporterPrivate::read_header()
  {
    // If read it before, we do need read it a second time.
    if ( this->read_header_ ) return true;

    if (! this->mrcreader_.read_header(this->importer_->get_filename(), this->header_) )
    {
      this->importer_->set_error(this->mrcreader_.get_error());
      return false;
    }

    switch (header_.mode)
    {
      case MRC2000IO::MRC_CHAR:
        this->data_type_ = Core::DataType::CHAR_E;
        break;
      case MRC2000IO::MRC_SHORT:
        this->data_type_ = Core::DataType::SHORT_E;
        break;
      case MRC2000IO::MRC_FLOAT:
        this->data_type_ = Core::DataType::FLOAT_E;
        break;
      default:
        this->importer_->set_error("Unsupported MRC format.");
        return false;
    }

    std::vector<size_t> dims(3);
    Core::Point origin;
    Core::Vector spacing;
    bool use_new_origin = this->mrcreader_.use_new_origin();

    // X=1, Y=2 and Z=3
    // axis corresponding to columns
    // fastest moving axis
    switch (this->header_.mapc) {
      case 1:
        dims[0] = header_.nx;
        spacing.x(header_.mx);
        if (use_new_origin)
        {
          origin.x(header_.xorigin);
        }
        else
        {
          origin.x(header_.nxstart);
        }
        break;
      case 2:
        dims[0] = header_.ny;
        spacing.x(header_.my);
        if (use_new_origin)
        {
          origin.x(header_.yorigin);
        }
        else
        {
          origin.x(header_.nystart);
        }
        break;
      case 3:
        dims[0] = header_.nz;
        spacing.x(header_.mz);
        if (use_new_origin)
        {
          origin.x(header_.zorigin);
        }
        else
        {
          origin.x(header_.nzstart);
        }
        break;
      default:
        this->importer_->set_error("Bad mapc axis value");
        return false;
    }

    // axis corresponding to rows
    // medium axis
    switch (header_.mapr) {
      case 1:
        dims[1] = header_.nx;
        spacing.y(header_.mx);
        if (use_new_origin)
        {
          origin.y(header_.xorigin);
        }
        else
        {
          origin.y(header_.nxstart);
        }
        break;
      case 2:
        dims[1] = header_.ny;
        spacing.y(header_.my);
        if (use_new_origin)
        {
          origin.y(header_.yorigin);
        }
        else
        {
          origin.y(header_.nystart);
        }
        break;
      case 3:
        dims[1] = header_.nz;
        spacing.y(header_.mz);
        if (use_new_origin)
        {
          origin.y(header_.zorigin);
        }
        else
        {
          origin.y(header_.nzstart);
        }
        break;
      default:
        this->importer_->set_error("Bad mapr axis value");
        return false;
    }

    // axis corresponding to sections
    // slowest moving axis
    switch (header_.maps) {
      case 1:
        dims[2] = header_.nx;
        spacing.z(header_.mx);
        if (use_new_origin)
        {
          origin.z(header_.xorigin);
        }
        else
        {
          origin.z(header_.nxstart);
        }
        break;
      case 2:
        dims[2] = header_.ny;
        spacing.z(header_.my);
        if (use_new_origin)
        {
          origin.z(header_.yorigin);
        }
        else
        {
          origin.z(header_.nystart);
        }
        break;
      case 3:
        dims[2] = header_.nz;
        spacing.z(header_.mz);
        if (use_new_origin)
        {
          origin.z(header_.zorigin);
        }
        else
        {
          origin.z(header_.nzstart);
        }
        break;
      default:
        this->importer_->set_error("Bad maps axis value");
        return false;
    }
    std::cout << "origin=[" << origin[0] << " " << origin[1] << " " << origin[2] << "]" << std::endl;
    std::cout << "spacing=[" << spacing[0] << " " << spacing[1] << " " << spacing[2] << "]" << std::endl;
    std::cout << "dims=[" << dims[0] << " " << dims[1] << " " << dims[2] << "]" << std::endl;
    Core::Transform transform(origin,
                              Core::Vector( spacing.x(), 0.0 , 0.0 ),
                              Core::Vector( 0.0, spacing.y(), 0.0 ),
                              Core::Vector( 0.0, 0.0, spacing.z() ));
    this->grid_transform_ = Core::GridTransform( dims[ 0 ], dims[ 1 ], dims[ 2 ], transform );
    this->grid_transform_.set_originally_node_centered( false );

    // Indicate that we read the header.
    this->read_header_ = true;
    
    return true;
  }
  
  bool MRCLayerImporterPrivate::read_data()
  {
    // Check if we already read the data.
    if ( this->read_data_ ) return true;
    
    // Ensure that we read the header of this file.
    if ( ! this->read_header() ) 
    {
      this->importer_->set_error( "Failed to read header of MRC file." );
      return false;
    }
    // Generate a new data block
    this->data_block_ = Core::StdDataBlock::New( this->grid_transform_.get_nx(), 
                                                 this->grid_transform_.get_ny(),
                                                 this->grid_transform_.get_nz(),
                                                 this->data_type_ );

    // We need to check if we could allocate the destination datablock
    if ( !this->data_block_ )
    {
      this->importer_->set_error( "Could not allocate enough memory to read MRC file." );
      return false;
    }

    // Now we read in the file
    // NOTE: We had to split this out due to a bug in Visual Studio's implementation for filestreams
    // These are unfortunately not 64bit compatible and hence use 32bit integers to denote offsets
    // into a file. Hence this breaks large data support. Hence in the next piece of code we deal
    // with windows separately.
    
#ifdef _WIN32
    HANDLE file_desc = CreateFileA( this->importer_->get_filename().c_str(), GENERIC_READ,
                                   FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if ( file_desc == INVALID_HANDLE_VALUE ) 
    {
      this->importer_->set_error( "Could not open file." );
      return false;
    }
#else
    std::ifstream data_file( this->importer_->get_filename().c_str(),
                             std::ios::in | std::ios::binary );
    if( !data_file )
    {
      this->importer_->set_error( "Could not open file." );
      return false;
    }
#endif
    
    // We start by getting the length of the entire file
#ifdef _WIN32
    LARGE_INTEGER offset; offset.QuadPart = 0;
    SetFilePointerEx( file_desc, offset, NULL, FILE_END);
#else
    data_file.seekg( 0, std::ios::end );
#endif
    
    // Next we compute the length of the data by subtracting the size of the header
    size_t file_size = 0;
    
#ifdef _WIN32
    offset.QuadPart = 0;
    LARGE_INTEGER cur_pos;
    SetFilePointerEx( file_desc, offset, &cur_pos, FILE_CURRENT );
    file_size = cur_pos.QuadPart; 
#else
    file_size = static_cast<size_t>( data_file.tellg() );
#endif
    
    // Ensure that the MRC file is of the right length
    size_t length = this->data_block_->get_size() * Core::GetSizeDataType( this->data_type_ );
    if ( file_size - MRC_HEADER_LENGTH < length )
    {
      this->importer_->set_error( "Incorrect length of file." );
      return false;
    }
    
    // We move the reader's position back to the front of the file and then to the start of the data
    //char* data = reinterpret_cast<char *>( this->data_block_->get_data() );
    char* data = new char[length];
    
#ifdef _WIN32
    offset.QuadPart = MRC_HEADER_LENGTH;
    SetFilePointerEx( file_desc, offset, NULL, FILE_BEGIN );
#else
    // test
    //data_file.seekg(0, std::ios::beg);
    //char* buffer = new char[MRC_HEADER_LENGTH];
    //data_file.read(buffer, MRC_HEADER_LENGTH);
    // test

    data_file.seekg( MRC_HEADER_LENGTH, std::ios::beg );
#endif
    
    // Finally, we do the actual reading and then save the data to the data block.
#ifdef _WIN32
    // NOTE: For windows we need to divide the read in chunks due to limitations in the
    // sizes we can store in the ReadFile function.
    char* data_ptr = data;
    DWORD dwReadBytes;
    size_t read_length = length;
    size_t chunk = 1UL<<30;
    
    while ( read_length > chunk )
    {
      ReadFile( file_desc, data_ptr, DWORD(chunk), &dwReadBytes, NULL );
      read_length -= static_cast<size_t>( dwReadBytes );
      data_ptr += static_cast<size_t>( dwReadBytes );
    }
    ReadFile( file_desc, data_ptr, DWORD( read_length ), &dwReadBytes, NULL );
    
#else
    data_file.read( data, length );
#endif
    
    // Close the file
#ifdef _WIN32
    CloseHandle( file_desc );
#else
    data_file.close();
#endif
    this->data_block_->set_data(data);

    // MRC data is always stored as big endian data
    // Hence we need to swap if we are on a little endian system.
    if ( this->mrcreader_.swap_endian() )
    {
      this->data_block_->swap_endian();
    }

    // Mark that we have read the data.
    this->read_data_ = true;
    
    // Done
    return true;
  }
  
  //////////////////////////////////////////////////////////////////////////
  // Class MRCLayerImporter
  //////////////////////////////////////////////////////////////////////////
  
  MRCLayerImporter::MRCLayerImporter() :
	private_( new MRCLayerImporterPrivate )
  {
    // Ensure that the private class has a pointer back into this class.
    this->private_->importer_ = this;
  }
  
  MRCLayerImporter::~MRCLayerImporter()
  {
  }
  
  bool MRCLayerImporter::get_file_info( LayerImporterFileInfoHandle& info )
  {
    try
    {	
      // Try to read the header
      if ( ! this->private_->read_header() ) return false;
      
      // Generate an information structure with the information.
      info = LayerImporterFileInfoHandle( new LayerImporterFileInfo );
      info->set_data_type( this->private_->data_type_ );
      info->set_grid_transform( this->private_->grid_transform_ );
      info->set_file_type( "mrc" ); 
      info->set_mask_compatible( true );
    }
    catch ( ... )
    {
      // In case something failed, recover from here and let the user
      // deal with the error. 
      this->set_error( "MRC Importer crashed while reading file." );
      return false;
    }
		
    return true;
  }
  
  
  bool MRCLayerImporter::get_file_data( LayerImporterFileDataHandle& data )
  {
    try
    {	
      // Read the data from the file
      if ( !this->private_->read_data() ) return false;
      
      // Create a data structure with handles to the actual data in this file	
      data = LayerImporterFileDataHandle( new LayerImporterFileData );
      data->set_data_block( this->private_->data_block_ );
      data->set_grid_transform( this->private_->grid_transform_ );
      data->set_name( this->get_file_tag() );
    }
    catch ( ... )
    {
      // In case something failed, recover from here and let the user
      // deal with the error. 
      this->set_error( "MRC Importer crashed when reading file." );
      return false;
    }
    
    return true;
  }
  
} // end namespace seg3D
