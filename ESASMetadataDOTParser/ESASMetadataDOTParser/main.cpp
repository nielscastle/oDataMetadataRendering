/*
 Original code by Castle+Andersen ApS (castleandersen.dk)
 
 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any
 damages arising from the use of this software.
 
 Permission is granted to anyone to use this software for any
 purpose, including commercial applications, and to alter it and
 redistribute it freely, subject to the following restrictions:
 
 1. The origin of this software must not be misrepresented; you must
 not claim that you wrote the original software. If you use this
 software in a product, an acknowledgment in the product documentation
 would be appreciated but is not required.
 
 2. Altered source versions must be plainly marked as such, and
 must not be misrepresented as being the original software.
 
 3. This notice may not be removed or altered from any source
 distribution.
 */

#include "tinyxml2.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

using namespace std;
using namespace filesystem;
using namespace tinyxml2;

/**
 * ALL THE BASIC EDM PROPERTIES
 */

struct EDMPropertyType {
    static constexpr auto Edmx = string_view{"edmx:Edmx"};
    static constexpr auto DataServices = string_view{"edmx:DataServices"};
    static constexpr auto Schema = string_view{"Schema"};
    static constexpr auto ComplexType = string_view{"ComplexType"};
    static constexpr auto Property = string_view{"Property"};
    static constexpr auto PropertyRef = string_view{"PropertyRef"};
    static constexpr auto Key = string_view{"Key"};
    static constexpr auto NavigationProperty = string_view{"NavigationProperty"};
    static constexpr auto EntityType = string_view{"EntityType"};
    static constexpr auto ReferentialConstraint = string_view{"ReferentialConstraint"};
    static constexpr auto EntitySet = string_view{"EntitySet"};
    static constexpr auto EntityContainer = string_view{"EntityContainer"};
};

/**
 * ALL THE BASIC EDM ATTRIBUTES
 */
struct EDMAttributeType {
    static constexpr auto Name = string_view{"Name"};
    static constexpr auto Type = string_view{"Type"};
    static constexpr auto Nullable = string_view{"Nullable"};
    static constexpr auto ContainsTarget = string_view{"ContainsTarget"};
    static constexpr auto ReferencedProperty = string_view{"ReferencedProperty"};
    static constexpr auto EntityType = string_view{"EntityType"};
    static constexpr auto Property = string_view{"Property"};
    static constexpr auto Version = string_view{"Version"};
    static constexpr auto Namespace = string_view{"Namespace"};
};

/**
 * Typedefs
 */
typedef map<string, string> Properties;
typedef vector<string> Strings;
typedef std::map<string, vector<string>> Associations;

/**
 * Helpers
 */

template <typename... Args>
void append_to_string( string &str, Args... args ) {
    ( str.append( args ), ... );
}

template <typename... Args>
void log( Args... args ) {
    ( ( cout << args ), ... );
}

struct Graph {

    void add_entity( const string &name, const string &type ) {
        for ( auto &[key, value] : associations ) {
            for ( auto &i : value ) {
                if ( i == type ) {
                    i = name;
                }
            }
        }
    }

    void add_table( const string &name, const map<string, string> &propterties ) {

        constexpr auto &border = "\'1\'";
        constexpr auto &cellBorder = "\'1\'";
        constexpr auto &color = "\'aliceblue\'";
        constexpr auto &bgcolor = "\'lightskyblue\'";
        constexpr auto &colSpan = "\'2'";

        auto newTable = string{};
        append_to_string( newTable, name,

                          " [\n rankdir=LR shape=plaintext\n label=<",
                          "<table border=", border,
                          " bgcolor=", bgcolor,
                          " cellborder=", cellBorder,
                          " color=", color,
                          ">",
                          " <tr><td colspan=", colSpan, ">", name, "</td></tr>" );

        if ( !propterties.empty() ) {
            for ( const auto &[value, key] : propterties ) {

                append_to_string( newTable, "\n<tr><td PORT=\"", value, "\" ALIGN=\"LEFT\">", value, "</td><td ALIGN=\"LEFT\">", key, "</td></tr>" );
            }
        }
        append_to_string( newTable, " </table>\n>]", " [fillcolor=aliceblue style=filled fontname=Helvetica];\n" );
        elements.emplace_back( newTable );
    }

    void create_arrows() {
        string newNode;
        for ( auto const &[key, value] : associations ) {
            for ( auto const &element : value ) {

                if ( element.find( "esas.Dynamics.Models.Contracts." ) != string::npos ) {
                    cout << "[ERROR]: found a non converted model => " << element << " skipping.." << endl;
                } else {
                    newNode = key + " -> " + element;
                    elements.push_back( newNode );
                }
            }
        }
    }

    void print_graph( ostream &stream ) {

        stream << "digraph Data {" << endl;

        for ( const auto &i : elements ) {
            stream << i << endl;
        }

        stream << "}" << endl;
    }

    map<string, string> find_all_properties( const XMLElement *element, const string &name ) {
        map<string, string> properties;
        for ( const XMLElement *child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement() ) {

            if ( child->Name() == EDMPropertyType::Property ) {
                properties[child->Attribute( EDMAttributeType::Name.data() )] = child->Attribute( EDMAttributeType::Type.data() );
            }

            if ( child->Name() == EDMPropertyType::NavigationProperty ) {

                string type = child->Attribute( EDMAttributeType::Type.data() );
                for ( const XMLElement *innerchild = child->FirstChildElement(); innerchild != nullptr; innerchild = innerchild->NextSiblingElement() ) {
                    if ( innerchild->Name() == EDMPropertyType::ReferentialConstraint ) {

                        string key{innerchild->Attribute( EDMAttributeType::Property.data() )};
                        string value{innerchild->Attribute( EDMAttributeType::ReferencedProperty.data() )};

                        string dest = type;
                        regex e{".*\\.(.+)$"};
                        if ( regex_match( dest, e ) ) {
                            dest = regex_replace( dest, e, "$1" );
                        }

                        associations[name + ":" + key].push_back( dest + ":" + value );
                    }
                }
            }
        }

        return properties;
    }

    void visit( const XMLElement *root ) {
        for ( const XMLElement *child = root->FirstChildElement(); child != nullptr; child = child->NextSiblingElement() ) {
            if ( child->Name() == EDMPropertyType::EntityType ) {
                auto name = child->Attribute( EDMAttributeType::Name.data() );
                auto properties = find_all_properties( child, name );
                add_table( name, properties );
            }

            if ( child->Name() == EDMPropertyType::EntitySet ) {
                auto name = child->Attribute( EDMAttributeType::Name.data() );
                auto type = child->Attribute( EDMAttributeType::EntityType.data() );
                add_entity( name, type );
            }

            visit( child );
        }
    }

    void removeAllEntitiesNotRelatedTo( string centerEntity ) {

        vector<string> relatedEnteties{centerEntity};

        for ( auto &[sourceField, targetFields] : associations ) {

            // Keep all relations going out of the center entity --

            if ( sourceField.rfind( centerEntity + ":", 0 ) == 0 ) {
                for ( const auto &field : targetFields ) {
                    relatedEnteties.emplace_back( entityFromFieldName( field ) );
                }
                continue;
            }

            // Remove all relations to other entities --

            targetFields.erase(
                remove_if( targetFields.begin(), targetFields.end(), [&]( const string &field ) { return field.rfind( centerEntity + ":", 0 ) != 0; } ),
                targetFields.end() );

            if ( targetFields.size() > 0 ) {
                relatedEnteties.emplace_back( entityFromFieldName( sourceField ) );
            }
        }

        // Delete all references that are not related to the center entity --

        for ( auto it = associations.begin(); it != associations.end(); ) {
            if ( find( relatedEnteties.begin(), relatedEnteties.end(), entityFromFieldName( it->first ) ) == relatedEnteties.end() ) {
                it = associations.erase( it );
            } else {
                ++it;
            }
        }

        // Delete all entities that are not related to the center entity --

        for ( auto it = elements.begin(); it != elements.end(); ) {

            bool found = false;
            for ( const auto &field : relatedEnteties ) {
                found |= ( *it ).find( field ) == 0;
            }

            if ( !found ) {
                it = elements.erase( it );
            } else {
                ++it;
            }
        }
    }

  private:
    vector<string> elements;
    Associations associations;

    string entityFromFieldName( string const &fieldName ) {
        std::string::size_type pos = fieldName.find( ':' );
        if ( pos != std::string::npos ) {
            return fieldName.substr( 0, pos );
        }
        return fieldName;
    }
};

auto main( int argc, char **argv ) -> int {

    if ( argc < 3 ) {
        cout << "Usage: prg <metadata file> <output dot file> <starting entity>" << endl;
        return 1;
    }

    auto xmlFileName = std::string_view{argv[1]}; // Input file
    auto dotFileName = std::string_view{argv[2]}; // Output file
    string centerEntity{};
    if(argc>3) {
        centerEntity = argv[3];                // Middle entity of graph, only include entities related to this
    }

    XMLDocument doc;
    doc.LoadFile( xmlFileName.data() );

    const XMLElement *root = doc.FirstChildElement( EDMPropertyType::Edmx.data() );
    if ( root == nullptr ) {
        cout << "Couldn't open input file " << path( xmlFileName ).native() << endl;
        return 1;
    }

    Graph graph;

    graph.visit( root );
    if(!centerEntity.empty()) {
     graph.removeAllEntitiesNotRelatedTo( centerEntity );
    }
    graph.create_arrows();

    ofstream myfile( dotFileName );
    if ( myfile.is_open() ) {
        cout << "Processing ..." << endl;
        graph.print_graph( myfile );
        cout << "Now use GraphViz to generate the diagram using fdp, dot, neato or equivalent:" << endl
             << endl;
        cout << "   /usr/local/bin/dot  -Tpdf " << dotFileName << "  -o /tmp/ER.pdf && open /tmp/ER.pdf" << endl
             << endl;
        cout << "Done." << endl;
        myfile.close();
    } else {
        cout << "Error writing to file!";
    }

    return 0;
}
