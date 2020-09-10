//
// Created by Orange Juice on 27-08-2020.
//

#ifndef XMLTODOTPARSER_XML_TO_DOT_HPP
#define XMLTODOTPARSER_XML_TO_DOT_HPP

#include <string>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>
#include <fplus/fplus.hpp>
#include "tinyxml2.h"

using namespace fplus;
using namespace tinyxml2;

#define LOG true
#define LOG_TO_FILE false
#define LOG_FILE_NAME "log.txt"

using XMLProperties = std::map<std::string, std::string>;
using dot_generated_code = std::vector<std::string>;
using XMLAssociations = std::map<std::string, std::vector<std::string>>;
using ExtractedNodes = std::vector<const XMLNode*>;
using ExtractedElements = std::vector<const XMLElement*>;
using ExtractedElementsList = std::vector<ExtractedElements>;
using XMLNames = std::vector<std::string>;
using XMLNamesList = std::vector<XMLNames>;
using SharedDocument = std::shared_ptr<XMLDocument>;

/**
 *
 * @tparam Args
 * @param args
 */

template<typename ...Args>
void log_string(Args&&... args)
{
    if constexpr(!LOG)
        return;

    if constexpr (LOG_TO_FILE){
        static std::ofstream file(LOG_FILE_NAME);
        if(file.is_open()){
            (file <<  ... <<  std::forward<Args>(args));
            file.close();
        }
    }else{
        (std::cout <<  ... <<  std::forward<Args>(args));
    }
}

/**
 *
 * @tparam Args
 * @param args
 * @return
 */

template<typename ...Args>
auto append_string(const Args& ...args) -> std::string
{
    return std::string{(args + ...)};
}

std::string error_message(const std::string& error_message){
    return append_string("[ERROR]: ", error_message);
}
std::string ok_message(const std::string& ok_message){
    return append_string("[OK]: ", ok_message);
}

/**
 * @param documentName => std::string
 * @return const XMLDocument * on success
 * @return std::string_view on error
 */
result<SharedDocument, std::string_view>
        read_xml_document(const std::string& documentName) {
    auto xml_document = std::make_shared<XMLDocument>();

    auto xml_error = xml_document->LoadFile(documentName.data());
    if ( xml_error == XMLError::XML_SUCCESS)
        return ok<SharedDocument, std::string_view>
                (xml_document);
    else
        return error <SharedDocument, std::string_view>
                (xml_document->ErrorStr());
}

/**
 *
 * @param start_node => const XMLNode* start_node
 * @param nodes => std::vector<const XMLNode*>
 */

void recursively_extract_all_nodes
    (const XMLNode* start_node, ExtractedNodes& nodes){
    for(auto node = start_node;
        node != nullptr;
        node = node->NextSibling()){
        nodes.emplace_back(node);
        recursively_extract_all_nodes(node->FirstChild(), nodes);
    }
}

/**
 * @param document => XMLDocument
 * @return ExtractedNodes => std::vector<const XMLNode*>
 */

result<ExtractedNodes, std::string_view >
        extract_all_nodes(const SharedDocument document){

    ExtractedNodes nodes{};
    recursively_extract_all_nodes(document->FirstChild(), nodes);

    log_string(ok_message("size of nodes to process -> "),
            nodes.size(),
            '\n');

    if(is_not_empty(nodes))
        return ok<ExtractedNodes, std::string_view>(nodes);
    else
        return error<ExtractedNodes, std::string_view>("No Nodes to extract!");
}

/**
 *
 * @param node => XMLNode*
 * @return ExtractedElements => std::vector<const XMLElement*>
 */
result<ExtractedElements , std::string_view >
        extract_all_elements_in_node(const XMLNode* node)
{
    ExtractedElements elements;
    for(auto element = node->FirstChildElement();
        element != nullptr;
        element = element->NextSiblingElement()){
        elements.emplace_back(element);
    }
    if(is_not_empty(elements))
        return ok<ExtractedElements, std::string_view>(elements);
    else
        return error<ExtractedElements, std::string_view>("No Elements to extract!");
}

/**
 *
 * @param extracted_nodes => std::vector<const XMLNode*>
 * @return ExtractedElementsList =>  std::vector<const ExtractedElements>
 */
result<ExtractedElementsList, std::string_view >
        extract_all_elements(const ExtractedNodes& extracted_nodes){
    ExtractedElementsList elements{};

    std::for_each(begin(extracted_nodes), end(extracted_nodes), [&](const auto& e){
        //for every node extract all the elements
        auto const result = extract_all_elements_in_node(e);
        if(is_ok(result))
            elements.emplace_back(unsafe_get_ok(result));
    });

    log_string(ok_message("number of extracted elements -> "),
               elements.size(),
               '\n');

    if(is_not_empty(elements))
        return ok < ExtractedElementsList, std::string_view > (elements);
    else
        return error < ExtractedElementsList, std::string_view > ("No element to extract");
}


/**
 *
 * @param element const XMLElement*
 * @return const std::string
 */

result<const std::string,  std::string_view > get_element_name(const XMLElement* element){
    if(element != nullptr)
        return ok<const std::string, std::string_view >(element->Name());
    else
        return error<const std::string, std::string_view >(" Name not Found! ");
}

/**
 *
 * @param element => std::vector<const XMLElement*>
 * @return XMLNames => std::vector<std::string>
 */

result<XMLNames, std::string_view >
        get_extracted_element(const ExtractedElements& element){
    XMLNames names = {};

    std::for_each(begin(element), end(element), [&](const auto& e){
        auto const result = get_element_name(e);
        if(is_ok(result))
            names.emplace_back(unsafe_get_ok(result));
        else
            log_string(error_message(unsafe_get_error(result).data()), '\n');
    });

    if(is_not_empty(names))
        return ok<XMLNames, std::string_view>(names);
    else
        return error<XMLNames, std::string_view>("No element found");

}

/**
 *
 * @param elements => std::vector<ExtractedElements>
 * @return XMLNamesList => std::vector<XMLNames>
 */

result<XMLNamesList, std::string_view >
    extract_all_element_names(const ExtractedElementsList& elements){
    XMLNamesList names = {};

    std::for_each(begin(elements), end(elements), [&](const auto& e){
        auto const result = get_extracted_element(e);
        if(is_ok(result))
            names.emplace_back(unsafe_get_ok(result));
        else
            log_string(error_message(unsafe_get_error(result).data()), '\n');
    });

    if(is_not_empty(names))
        return ok<XMLNamesList, std::string_view >(names);
    else
        return error<XMLNamesList, std::string_view >("No names are found");

};

/**
 *
 * @param args
 * @param argc
 * @return
 */
result<dot_generated_code, std::string_view>
        generate_dot_code()
{
}

/**
 *
 * @param xmlFileName
 * @return
 */


template<typename Container>
void print_vector(Container c){
    const auto result = join_result(c);
    if(is_ok(result)){
        for(const auto& item : unsafe_get_ok(result)){
            log_string(show_cont(item));
        }
    }else{
        log_string(error_message(unsafe_get_error(result).data()));
    }
}

auto run(std::string_view xmlFileName) -> int{

    log_string(ok_message("running...\n"));
    const auto xml_doc = read_xml_document(xmlFileName.data());

    const auto get_all_names = compose_result(extract_all_nodes,
            extract_all_elements,
            extract_all_element_names);

    const auto names = fwd::apply(xml_doc,
                                  fwd::lift_result(get_all_names));
    print_vector(names);


    //const auto get_all_entity_nodes = compose_result(extract_all_nodes, extract_entity_nodes);
    //TODO: generate a map where map[entity] = (ModelName,[ProppertyNames], [XMLAssociations])
    //const auto generate_entity_map = compose_result(get_all_entity_nodes, )

    //const auto result = unify_result(, error_message, names);

    return 0;
}



#endif //XMLTODOTPARSER_XML_TO_DOT_HPP
