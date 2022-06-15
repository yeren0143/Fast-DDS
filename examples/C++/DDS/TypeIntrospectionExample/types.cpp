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
 * @file types.cpp
 *
 */

#include <fastrtps/types/DynamicDataHelper.hpp>
#include <fastrtps/types/DynamicDataFactory.h>
#include <fastrtps/types/TypeObjectFactory.h>
#include <fastrtps/types/TypesBase.h>

#include "types.hpp"

using namespace eprosima::fastrtps::types;


void print_dynamic_data(
        const DynamicData_ptr& dyn_data)
{
    DynamicDataHelper::print(dyn_data);
}

std::ostream& operator <<(
        std::ostream& output,
        const TypeKind& kind)
{
    switch(kind)
    {
        case TK_NONE:
        {
            std::cout << "< NONE >";
            break;
        }
        case TK_BOOLEAN:
        {
            std::cout << "< BOOLEAN >";
            break;
        }
        case TK_BYTE:
        {
            std::cout << "< BYTE >";
            break;
        }
        case TK_INT16:
        {
            std::cout << "< INT16 >";
            break;
        }
        case TK_INT32:
        {
            std::cout << "< INT32 >";
            break;
        }
        case TK_INT64:
        {
            std::cout << "< INT64 >";
            break;
        }
        case TK_UINT16:
        {
            std::cout << "< UINT16 >";
            break;
        }
        case TK_UINT32:
        {
            std::cout << "< UINT32 >";
            break;
        }
        case TK_UINT64:
        {
            std::cout << "< UINT64 >";
            break;
        }
        case TK_FLOAT32:
        {
            std::cout << "< FLOAT32 >";
            break;
        }
        case TK_FLOAT64:
        {
            std::cout << "< FLOAT64 >";
            break;
        }
        case TK_FLOAT128:
        {
            std::cout << "< FLOAT128 >";
            break;
        }
        case TK_CHAR8:
        {
            std::cout << "< CHAR8 >";
            break;
        }
        case TK_CHAR16:
        {
            std::cout << "< CHAR16 >";
            break;
        }
        case TK_STRING8:
        {
            std::cout << "< STRING8 >";
            break;
        }
        case TK_STRING16:
        {
            std::cout << "< STRING16 >";
            break;
        }
        case TK_BITMASK:
        {
            std::cout << "< BITMASK >";
            break;
        }
        case TK_ENUM:
        {
            std::cout << "< ENUM >";
            break;
        }
        case TK_STRUCTURE:
        {
            std::cout << "< STRUCTURE >";
            break;
        }
        case TK_BITSET:
        {
            std::cout << "< BITSET >";
            break;
        }
        case TK_UNION:
        {
            std::cout << "< UNION >";
            break;
        }
        case TK_SEQUENCE:
        {
            std::cout << "< SEQUENCE >";
            break;
        }
        case TK_ARRAY:
        {
            std::cout << "< ARRAY >";
            break;
        }
        case TK_MAP:
        {
            std::cout << "< MAP >";
            break;
        }
        default:
        {
            std::cout << "< UNKNOWN >";
            break;
        }
    }

    return output;
}

std::ostream& operator <<(
        std::ostream& output,
        DynamicTypeMember* member_type)
{
    TypeKind kind = member_type->get_descriptor()->get_kind();

    // Print kind
    output << kind;

    switch(kind)
    {
        // TODO
        case TK_STRUCTURE:
        case TK_BITSET:
        case TK_UNION:
        {
            std::map<std::string, DynamicTypeMember*> members;
            member_type->get_descriptor()->get_type()->get_all_members_by_name(members);

            output << " { ";
            for (auto it : members)
            {
                output << it.first << ": " << it.second;
            }
            output << " } ";
            break;
        }

        // case TK_SEQUENCE:
        // case TK_ARRAY:
        // {
        //     DynamicData* st_data = data->loan_value(type->get_id());
        //     print_collection(st_data, tabs + "\t");
        //     data->return_loaned_value(st_data);
        //     break;
        // }

        // case TK_MAP:
        // {
        //     DynamicData* st_data = data->loan_value(type->get_id());
        //     std::map<MemberId, DynamicTypeMember*> members;
        //     desc->get_type()->get_all_members(members);
        //     size_t size = data->get_item_count();
        //     for (size_t i = 0; i < size; ++i)
        //     {
        //         size_t index = i * 2;
        //         MemberId id = data->get_member_id_at_index(static_cast<uint32_t>(index));
        //         std::cout << "Key: ";
        //         print_member(st_data, members[id], tabs + "\t");
        //         id = data->get_member_id_at_index(static_cast<uint32_t>(index + 1));
        //         std::cout << "Value: ";
        //         print_member(st_data, members[id], tabs + "\t");
        //     }
        //     data->return_loaned_value(st_data);
        //     break;
        // }

        default:
            break;
    }

    return output;
}

std::ostream& operator <<(
        std::ostream& output,
        const DynamicType_ptr& dyn_type)
{
    // Get members from type
    std::map<std::string, DynamicTypeMember*> members;
    dyn_type->get_all_members_by_name(members);

    // Print members
    for (auto it : members)
    {
        output << it.first << ": " << it.second;
        output << "\n";
    }

    return output;
}
