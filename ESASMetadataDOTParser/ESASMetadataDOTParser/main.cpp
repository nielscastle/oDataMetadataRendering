/*
Original code by Castle+Andersen ApS (www.castleandersen.dk)

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
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>
#include <filesystem>

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

Associations associations;

const vector<string> skipList = {
    //    "Adgangskrav",
    //    "Eksamenstype",
    "Afdeling",
    //    "Afslagsbegrundelse",
    //    "Postnummer",
    //    "Land",
    //    "Branche",
    "Aktivitetsudbud",
    //    "Ansoeger",
    //    "Ansoegning",
    //    "Ansoegningshandling",
    //    "Ansoegningskort",
    //    "AnsoegningskortOpsaetning",
    //    "AnsoegningskortTekst",
    //    "Ansoegningsopsaetning",
    //    "Bedoemmelse",
    //    "Bedoemmelsesrunde",
    //    "Bevisgrundlag",
    //    "Bilag",
    "Fagpersonsrelation",
    //    "Fravaersaarsag",
    //    "Gebyrtype",
    "GennemfoerelsesUddannelseselement",
    //    "GymnasielleFagkrav",
    //    "GymnasielleKarakterkrav",
    "Hold",
    //    "Institutionsoplysninger",
    "InstitutionVirksomhed",
    //    "Internationalisering",
    //    "Karakter",
    //    "Kommunikation",
    //    "Kvalifikationskriterie",
    //    "Kvalifikationspoint",
    //    "MeritRegistrering",
    //    "NationalAfgangsaarsag",
    //    "Omraadenummer",
    //    "Omraadenummeropsaetning",
    //    "Omraadespecialisering",
    //    "OptionSetValueString",
    "Person",
    "Personoplysning",
    "PlanlaegningsUddannelseselement",
    //    "Praktikomraade",
    //    "Praktikophold",
    //    "RelationsStatus",
    //    "Publicering",
    "StruktureltUddannelseselement",
    "Studieforloeb",
    //    "StudieinaktivPeriode",
    //    "SystemUser",
    "Uddannelsesstruktur",
    "Uddannelsesaktivitet",
    //    "Enkeltfag",
    //    "Specialisering",
    //    "AndenAktivitet",
    //    "Erfaringer",
    //    "KOTGruppe",
    //    "KOTGruppeTilmelding",
    //    "KurserSkoleophold",
    //    "Proeve",
    //    "UdlandsopholdAnsoegning",
    //    "VideregaaendeUddannelse",
};

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

    explicit Graph( const string &name ) {
        auto start = string{};
        append_to_string( start, "digraph ", name, "{" );
        elements.emplace_back( start );
    }

    void end() {
        elements.emplace_back( "}" );
    }

    static void add_entity( const string &name, const string &type ) {
        for ( auto &[key, value] : associations ) {
            for ( auto &i : value ) {
                if ( i == type ) {
                    i = name;
                }
            }
        }
    }

    void add_table( const string &name, const map<string, string> &propterties ) {

        if ( find( skipList.begin(), skipList.end(), name ) == skipList.end() ) {
            return;
        }

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
        for ( const auto &i : elements )
            stream << i << endl;
    }

  private:
    vector<string> elements;
};

static Graph graph( "Data" );

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

                    if ( find( skipList.begin(), skipList.end(), name ) == skipList.end() ) {
                        continue;
                    }
                    if ( find( skipList.begin(), skipList.end(), dest ) == skipList.end() ) {
                        continue;
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
            graph.add_table( name, properties );
        }

        if ( child->Name() == EDMPropertyType::EntitySet ) {
            auto name = child->Attribute( EDMAttributeType::Name.data() );
            auto type = child->Attribute( EDMAttributeType::EntityType.data() );
            graph.add_entity( name, type );
        }

        visit( child );
    }
}

auto main( int argc, char **argv ) -> int {

    auto xmlFileName = std::string_view{argv[1]}; // Input file
    auto dotFileName = std::string_view{argv[2]}; // Output file

    XMLDocument doc;
    doc.LoadFile( xmlFileName.data() );

    const XMLElement *root = doc.FirstChildElement( EDMPropertyType::Edmx.data() );
    if ( root == nullptr ) {
        cout << "Couldn't open input file " << path(xmlFileName).native()       << endl;
        return 1;
    }

    visit( root );
    graph.create_arrows();
    graph.end();

    ofstream myfile( dotFileName );
    if ( myfile.is_open() ) {
        cout << "Processing ..." << endl;
        graph.print_graph( myfile );
        cout << "Now use GraphViz to generate the diagram using:" << endl<<endl;
        cout << "   /usr/local/bin/dot  -Tpdf " << dotFileName << "  -o /tmp/ER.pdf && open /tmp/ER.pdf" << endl << endl;
        cout << "Done." << endl;
        myfile.close();
    } else
        cout << "Error writing to file!";
    return 0;
}
