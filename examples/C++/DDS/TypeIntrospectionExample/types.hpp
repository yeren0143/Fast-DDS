// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file types.h
 *
 */

#ifndef _EPROSIMA_FASTDDS_EXAMPLES_CPP_DDS_TYPEINTROSPECTIONEXAMPLE_TYPES_HPP_
#define _EPROSIMA_FASTDDS_EXAMPLES_CPP_DDS_TYPEINTROSPECTIONEXAMPLE_TYPES_HPP_

#include <ostream>

#include <fastrtps/types/DynamicDataHelper.hpp>
#include <fastrtps/types/DynamicDataFactory.h>
#include <fastrtps/types/TypeObjectFactory.h>
#include <fastrtps/types/TypesBase.h>

enum class DataType
{
    HELLOWORLD,  // name -> HelloWorld
    DATA_POINT_XML,  // name -> DataPointXML
};

constexpr const char * HELLO_WORLD_DATA_TYPE = "hw";
constexpr const char * DATA_POINT_XML_DATA_TYPE = "xml";

// Stream of a dynamic type. Shows each field of the type with its name and data type.
std::ostream& operator <<(
        std::ostream& output,
        const eprosima::fastrtps::types::DynamicType_ptr& dyn_type);

// Stream of a dynamic data. Shows each field of the data with its name and value.
void print_dynamic_data(
        const eprosima::fastrtps::types::DynamicData_ptr& dyn_data);

#endif /* _EPROSIMA_FASTDDS_EXAMPLES_CPP_DDS_TYPEINTROSPECTIONEXAMPLE_TYPES_HPP_ */
