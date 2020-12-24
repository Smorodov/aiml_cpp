#include "pugixml.hpp"

#include <tinyutf8.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>
#include <map>
#include <vector>
#include <locale>

#ifdef _WIN32
#include <windows.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>
#endif
//----------------------------------------
//
//----------------------------------------
void toLower(tiny_utf8::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::towlower);
}
//----------------------------------------
//
//----------------------------------------
void toUpper(tiny_utf8::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::towupper);
}
//----------------------------------------
//
//----------------------------------------
void split(std::vector<tiny_utf8::string>& theStringVector,  // Altered/returned value
    const  tiny_utf8::string& theString,
    const  tiny_utf8::string& theDelimiter)
{
    std::vector<tiny_utf8::string> stringVector;
    size_t  start = 0, end = 0;
    while (end != tiny_utf8::string::npos)
    {
        end = theString.find(theDelimiter, start);

        // If at end, use length=maxLength.  Else use length=end-start.
        stringVector.push_back(theString.substr(start,
            (end == tiny_utf8::string::npos) ? tiny_utf8::string::npos : end - start));

        // If at end, use start=maxSize.  Else use start=end+delimiter.
        start = ((end > (tiny_utf8::string::npos - theDelimiter.size()))
            ? tiny_utf8::string::npos : end + theDelimiter.size());
    }

    theStringVector.clear();
    for (auto s : stringVector)
    {
        if (s != "")
        {
            theStringVector.push_back(s);
        }
    }

}
//----------------------------------------
//
//----------------------------------------
tiny_utf8::string join(std::vector<tiny_utf8::string>& strList, tiny_utf8::string separator)
{
    tiny_utf8::string result;
    std::vector<tiny_utf8::string>::iterator it;
    for (it = strList.begin(); it != strList.end(); ++it)
    {
        result += *it;
        if (it < strList.end() - 1)
        {
            result += separator;
        }
    }
    return result;
}
//----------------------------------------
//
//----------------------------------------
void trimString(tiny_utf8::string& str)
{
    int pos = str.find_last_not_of(U" ");
    if (pos != -1)
    {
        str.erase(str.begin() + pos + 1, str.end());
    }

    pos = str.find_first_not_of(U" ");
    if (pos != -1)
    {
        str.erase(str.begin(), str.begin() + pos);
    }

}
//----------------------------------------
//
//----------------------------------------
void replace(tiny_utf8::string& str, tiny_utf8::string from, tiny_utf8::string to)
{
    for (int pos = str.find(from); pos != -1; pos = str.find(from))
    {
        str.erase(str.begin() + pos, str.begin() + pos + from.length());
        if (to != "")
        {
            str.insert(str.begin() + pos, to);
        }
    }
}
//----------------------------------------
//
//----------------------------------------
void normalizeString(tiny_utf8::string& str)
{
    replace(str, U"\n", " ");
    std::vector<int> spacesPos;
    for (int pos = str.find_first_of(U" "); pos != -1; pos = str.find_first_of(U"  "))
    {
        int end_pos = str.find_first_not_of(U"  ", pos);
        str.erase(str.begin() + pos, str.begin() + end_pos);
        spacesPos.push_back(pos);
    }
    for (int i = 0; i < spacesPos.size(); ++i)
    {
        str.insert(str.begin() + spacesPos[i] + i, " ");
    }

    trimString(str);
    toLower(str);
}
//----------------------------------------
//
//----------------------------------------
tiny_utf8::string fill(tiny_utf8::string s, int n)
{
    tiny_utf8::string res = "";
    for (int i = 0; i < n; ++i)
    {
        res = res.append(s);
    }
    return res;
}

//----------------------------------------
//
//----------------------------------------
//A much faster replacement of regExp.exactMatch(str)
//it also captures the words corresponding to the wildcards * & _
bool exactMatch(tiny_utf8::string regExp, tiny_utf8::string str, std::vector<tiny_utf8::string>& capturedText)
{
    std::vector<tiny_utf8::string> regExpWords;
    split(regExpWords, regExp, U" ");
    std::vector<tiny_utf8::string> strWords;
    split(strWords, str, U" ");

    for (auto re : regExpWords)
    {
        std::cout << u8"Точный шаблон :" << re.c_str() << std::endl;
    }

    if ((regExpWords.empty() || strWords.empty()) &&
        (regExpWords.size() != strWords.size()))
    {
        return false;
    }

    if (regExpWords.size() > strWords.size())
    {
        return false;
    }

    std::vector<tiny_utf8::string>::const_iterator regExpIt = regExpWords.begin();
    std::vector<tiny_utf8::string>::const_iterator strIt = strWords.begin();
    while ((strIt != strWords.end()) && (regExpIt != regExpWords.end()))
    {
        if ((*regExpIt == U"*") || (*regExpIt == U"_"))
        {
            regExpIt++;
            std::vector<tiny_utf8::string> capturedStr;
            if (regExpIt != regExpWords.end())
            {
                tiny_utf8::string nextWord = *regExpIt;
                if ((nextWord != U"*") && (nextWord != U"_"))
                {
                    while (true)
                    {
                        if (*strIt == nextWord)
                        {
                            break;
                        }
                        capturedStr.push_back(*strIt);
                        if (++strIt == strWords.end())
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    capturedStr.push_back(*strIt);
                    regExpIt--;
                }
            }
            else
            {
                while (true)
                {
                    capturedStr.push_back(*strIt);
                    if (++strIt == strWords.end())
                    {
                        break;
                    }
                }

                capturedText.push_back(join(capturedStr, " "));
                return true;
            }
            capturedText.push_back(join(capturedStr, " "));
        }
        else if (*regExpIt != *strIt)
        {
            return false;
        }
        regExpIt++;
        strIt++;
    }
    return (regExpIt == regExpWords.end()) && (strIt == strWords.end());
}
//----------------------------------------
//
//----------------------------------------
#define MAX_LIST_LENGTH 50
#define MAX_RECURSION   50
struct Node;
struct Leaf
{
    Node* parent;
    pugi::xml_node tmplate;
    tiny_utf8::string topic;
    tiny_utf8::string that;
    Leaf()
    {
        topic = that = "";
    }

};
//----------------------------------------
//
//----------------------------------------
struct Node
{
    Node* parent;
    tiny_utf8::string word;
    std::vector<Node*> childs;
    std::vector<Leaf*> leafs;
    //----------------------------------------
    //
    //----------------------------------------
    Node()
    {
        word = "";
    }
    //----------------------------------------
    //
    //----------------------------------------
    ~Node()
    {
        clear();
    }
    //----------------------------------------
    //
    //----------------------------------------
    void clear()
    {
        std::vector<Node>::iterator it;
        for (std::vector<Node*>::iterator it = childs.begin(); it != childs.end(); ++it)
        {
            delete* it;
        }
        childs.clear();

        for (std::vector<Leaf*>::iterator it = leafs.begin(); it != leafs.end(); ++it)
        {
            delete* it;
        }
        leafs.clear();
    }
    //----------------------------------------
    //
    //----------------------------------------
    bool match(std::vector<tiny_utf8::string>::const_iterator input,
        const std::vector<tiny_utf8::string>& inputWords,
        const tiny_utf8::string& currentThat,
        const tiny_utf8::string& currentTopic,
        std::vector<tiny_utf8::string>& capturedThatTexts,
        std::vector<tiny_utf8::string>& capturedTopicTexts,
        Leaf*& leaf);
};

//----------------------------------------
//
//----------------------------------------
bool Node::match(std::vector<tiny_utf8::string>::const_iterator input,
    const std::vector<tiny_utf8::string>& inputWords,
    const tiny_utf8::string& currentThat,
    const tiny_utf8::string& currentTopic,
    std::vector<tiny_utf8::string>& capturedThatTexts,
    std::vector<tiny_utf8::string>& capturedTopicTexts,
    Leaf*& leaf)
{
    if (input == inputWords.end())
    {
        return false;
    }

    //std::cout << "Match: " << word << std::endl;

    if ((word == U"*") || (word == U"_"))
    {
        ++input;
        for (; input != inputWords.end(); input++)
        {
            for (Node* child : childs)
            {
                if (child->match(input, inputWords, currentThat, currentTopic, capturedThatTexts,
                    capturedTopicTexts, leaf))
                {
                    return true;
                }
            }
        }
    }
    else
    {
        if (!word.empty())
        {
            if (word != *input)
            {
                return false;
            }
            ++input;
        }
        for (Node* child : childs)
        {
            if (child->match(input, inputWords, currentThat, currentTopic, capturedThatTexts,
                capturedTopicTexts, leaf))
            {
                return true;
            }
        }
    }
    if (input == inputWords.end())
    {
        for (int i = 0; i < leafs.size(); ++i)
        {
            leaf = leafs[i];
            capturedThatTexts.clear();
            capturedTopicTexts.clear();
            if ((!leaf->that.empty() && !exactMatch(leaf->that, currentThat, capturedThatTexts)) ||
                (!leaf->topic.empty() && !exactMatch(leaf->topic, currentTopic, capturedTopicTexts)))
            {
                continue;
            }
            return true;
        }
    }

    return false;
}
//----------------------------------------
//
//----------------------------------------
std::vector<pugi::xml_node> elementsByTagName(pugi::xml_node* node, const tiny_utf8::string& tagName)
{
    std::vector<pugi::xml_node> list;

    pugi::xml_node n = *node;
    while (!n.empty())
    {
        //std::cout << " inside " << n.name() << std::endl;
        if (n.name() == std::string(tagName.c_str()))
        {
            list.push_back(n);
        }
        pugi::xml_node nc = n.first_child();
        if (!nc.empty())
        {
            std::vector<pugi::xml_node> res = elementsByTagName(&nc, tagName);
            list.insert(list.end(), res.begin(), res.end());
        }
        n = n.next_sibling();
    }
    return list;
}
//----------------------------------------
//
//----------------------------------------

//----------------------------------------
//
//----------------------------------------
//----------------------------------------
//
//----------------------------------------
pugi::xml_document doc;
std::stringstream logStream;
std::vector<tiny_utf8::string> inputList;
std::vector<std::vector<tiny_utf8::string>> thatList;
std::map<tiny_utf8::string, tiny_utf8::string > parameterValue;
std::map<tiny_utf8::string, tiny_utf8::string> botVarValue;
std::vector<tiny_utf8::string> subOld;
std::vector<tiny_utf8::string> subNew;
Node root;
int indent;
bool displayTree;
short blockSize;
std::string currentPath;

bool loadAiml(const std::string&);
//tiny_utf8::string executeCommand(const tiny_utf8::string& commandStr);
tiny_utf8::string getResponse(tiny_utf8::string, const bool& = false);
tiny_utf8::string resolveNode(pugi::xml_node*, const std::vector<tiny_utf8::string> & = std::vector<tiny_utf8::string>(),
    const std::vector<tiny_utf8::string> & = std::vector<tiny_utf8::string>(), const std::vector<tiny_utf8::string> & = std::vector<tiny_utf8::string>());


//----------------------------------------
//
//----------------------------------------
bool loadSubstitutions(const tiny_utf8::string& filename)
{
    pugi::xml_document doc;
    std::ifstream stream(filename.c_str());
    pugi::xml_parse_result result = doc.load(stream);
    if (result)
    {
        std::cout << "Substitutions XML parsed without errors";
    }
    else
    {
        std::cout << "Substitutions XML parsed with errors" << std::endl;
        return false;
    }
    std::vector<pugi::xml_node> subsList = elementsByTagName(&doc.first_child(), U"substitution");
    subOld.clear();
    subNew.clear();
    for (int i = 0; i < subsList.size(); i++)
    {
        pugi::xml_node n = subsList[i];

        subOld.push_back(n.child("old").text().as_string());
        subNew.push_back(n.child("new").text().as_string());
    }
    return true;
}
//----------------------------------------
//
//----------------------------------------
bool loadVars(const tiny_utf8::string& filename, const bool& bot)
{
    pugi::xml_document doc;
    std::ifstream stream(filename.c_str());
    pugi::xml_parse_result result = doc.load(stream);
    if (result)
    {
        std::cout << "Substitutions XML parsed without errors";
    }
    else
    {
        std::cout << "Substitutions XML parsed with errors" << std::endl;
        return false;
    }
    std::vector<pugi::xml_node> setList = elementsByTagName(&doc.child("vars"), U"set");

    for (int i = 0; i < setList.size(); i++)
    {
        pugi::xml_node n = setList[i];

        tiny_utf8::string name = n.attribute("name").as_string();
        tiny_utf8::string value = n.text().as_string();
        std::cout << " name :" << name << " value :" << value << std::endl;
        if (bot)
            botVarValue[name] = value;
        else
            parameterValue[name] = value;
    }
    return true;
}
//----------------------------------------
//
//----------------------------------------
bool saveVars(const std::string& filename)
{    
    pugi::xml_document doc;
    pugi::xml_node root=doc.append_child("vars");
    std::map<tiny_utf8::string, tiny_utf8::string>::const_iterator it;
    for (it = parameterValue.begin(); it != parameterValue.end(); ++it)
    {
        pugi::xml_node setElem = root.append_child("set");
        pugi::xml_attribute name_attr=setElem.append_attribute("name");// , it.key());
        name_attr.set_name(it->first.c_str());
        name_attr.set_value(it->second.c_str());
    }
    std::ofstream stream(filename.c_str());
    doc.save(stream);
    return true;
}


//----------------------------------------
//
//----------------------------------------
void init()
{
    srand(time(NULL));
    indent = 0;
    root.parent = NULL;
    loadSubstitutions("data/substitutions.xml");
    loadVars("data/vars.xml",false);
    loadVars("data/bot.xml", true);
}
//----------------------------------------
//
//----------------------------------------
void clear()
{
    saveVars("data/vars.xml");
    inputList.clear();
    thatList.clear();
    indent = 0;
    root.clear();
}
//----------------------------------------
//
//----------------------------------------
tiny_utf8::string executeCommand(const tiny_utf8::string& commandStr)
{
    tiny_utf8::string returnString(U"");
    tiny_utf8::string spaceIndent = fill(U" ", 2 * indent);
    logStream << spaceIndent << "Executing \"" << commandStr << "\" ...\n";
#ifdef _WIN32
    STARTUPINFO si;
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    PROCESS_INFORMATION pi;
    HANDLE read_pipe, write_pipe;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    int fd, create;
    OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(osv);

    GetVersionEx(&osv);

    if (osv.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
        sa.lpSecurityDescriptor = &sd;
    }
    else // Pipe will use ACLs from default descriptor
        sa.lpSecurityDescriptor = NULL;

    // Create a new pipe with system's default buffer size
    if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0))
    {
        logStream << spaceIndent + "Can't create pipe!\n";
        return "";
    }

    GetStartupInfo(&si);

    // Sets the standard output handle for the process to the
    // handle specified in hStdOutput 
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = write_pipe;
    si.hStdError = (HANDLE)_get_osfhandle(2);
    si.wShowWindow = 0;
    tiny_utf8::string str = U"cmd.exe /c \"" + commandStr + U"\"";
    create = CreateProcessA(NULL,                                   // The full path of app to launch
        LPSTR(str.c_str()),  // Command line parameters
        NULL,                                   // Default process security attributes
        NULL,                                   // Default thread security attributes
        TRUE,                                   // Inherit handles from the parent
        0,                                      // Normal priority
        NULL,                                   // Use the same environment as the parent
        NULL,                                   // Use app's directory as current
        LPSTARTUPINFOA(&si),                                    // Startup Information
        &pi);                                   // Process information stored upon return
    if (!create)
    {
        logStream << "Cam't create prpcess!\n";
        return "";
    }

    /* Associates a file descriptor with the stdout pipe */
    fd = _open_osfhandle(/*(intptr_t)*/(long int)read_pipe, _O_BINARY);

    /* Close the handle that we're not going to use */
    CloseHandle(write_pipe);
    if (!fd)
    {
        logStream << "Can't open pipe for write !\n";
        return "";
    }

    /* Open the pipe stream using its file descriptor */
    FILE* file = fdopen(fd, "r");
    if (!file)
    {
        logStream << "Can't open file stream !\n";
        return "";
    }
#else

    FILE* file = popen(qPrintable(commandStr), "r");
    if (!file)
    {
        logStream << "Execution failed !\n";
        return _T("");
    }
#endif

    char c = 0;
    while (c != EOF)
    {
        c = (char)getc(file);

        if (isascii(c))
            returnString += c;
    }

    fclose(file);

    logStream << U"Execution succeeded with result: \"" + returnString + U"\"\n";
    return returnString;
}
//----------------------------------------
//
//----------------------------------------
//recursively replace all the values & return the QString result
tiny_utf8::string resolveNode(pugi::xml_node* node, const std::vector<tiny_utf8::string>& capturedTexts,
    const std::vector<tiny_utf8::string>& capturedThatTexts, const std::vector<tiny_utf8::string>& capturedTopicTexts)
{
    tiny_utf8::string result = "";
    tiny_utf8::string nodeName = node->name();
    //std::cout << u8"Обработка узла " << nodeName.c_str() << std::endl;

    if (nodeName == U"random")
    {
        std::vector<pugi::xml_node> childNodes = elementsByTagName(node, "li");
        unsigned int childCount = childNodes.size();
        unsigned int random = rand() % childCount;
        pugi::xml_node child = childNodes[random];
        result = resolveNode(&child, capturedTexts, capturedThatTexts, capturedTopicTexts);
    }

    else if (nodeName == U"condition")
    {
        tiny_utf8::string name("");
        unsigned int condType = 2;
        if (!node->attribute("name").empty())
        {
            condType = 1;
            name = node->attribute("name").as_string();
            if (!node->attribute("value").empty())
            {
                condType = 0;
                tiny_utf8::string value = node->attribute("value").as_string();
                toUpper(value);
                std::vector<tiny_utf8::string> dummy;
                tiny_utf8::string paramVal = parameterValue[name];
                toUpper(paramVal);
                if (exactMatch(value, paramVal, dummy))
                {
                    //dirty trick to avoid infinite loop !
                    node->set_name("parsedCondition");
                    result = resolveNode(node, capturedTexts, capturedThatTexts, capturedTopicTexts);
                    node->set_name("condition");
                }
            }
        }

        if (condType)
        {
            std::vector<pugi::xml_node> childNodes = elementsByTagName(node, "li");
            for (unsigned int i = 0; i < childNodes.size(); i++)
            {
                pugi::xml_node n = childNodes[i];
                if (!n.attribute("value").empty())
                {
                    if (condType == 2)
                        name = n.attribute("name").as_string();

                    tiny_utf8::string value = n.attribute("value").as_string();
                    toUpper(value);
                    std::vector<tiny_utf8::string> dummy;
                    tiny_utf8::string paramVal = parameterValue[name];
                    toUpper(paramVal);
                    if (exactMatch(value, paramVal, dummy))
                    {
                        result = resolveNode(&n, capturedTexts, capturedThatTexts, capturedTopicTexts);
                        break;
                    }
                }
                else
                {
                    result = resolveNode(&n, capturedTexts, capturedThatTexts, capturedTopicTexts);
                    break;
                }
            }
        }
    }
    else
    {
        bool htmlTag = false;
        if (nodeName.starts_with("html:"))
        {
            tiny_utf8::string xmlTag;
            std::stringstream ts;
            htmlTag = true;
            ts << '<' << nodeName;
            const auto attributes = node->attributes();
            for (auto attribute : attributes)
            {
                ts << ' ';
                ts << attribute.as_string();
            }
            ts << '>';

            result += xmlTag;
        }

        pugi::xml_node n = node->first_child();
        bool ready = false;
        //std::cout << "Node name :" << nodeName << std::endl;


        while (!n.empty())
        {
            tiny_utf8::string ans = resolveNode(&n, capturedTexts, capturedThatTexts, capturedTopicTexts);
            result += U" " + ans;
            //std::cout << "result :" << result << std::endl;
            n = n.next_sibling();
            if (nodeName == "template")
            {
                ready = true;
            }
        }
        if (ready)
        {
            return result;
        }

        if (htmlTag)
        {
            result += U"</" + nodeName + U">";
            return result;
        }
        else if (!node->text().empty())
        {
            result = node->text().as_string();
        }
        else if (nodeName == U"set")
        {
            trimString(result);
            parameterValue[node->attribute("name").as_string()] = result;
        }
        else if (nodeName == U"srai")
            result = getResponse(result, true);
        else if (nodeName == U"think")
            result = "";
        else if (nodeName == U"system")
        {
            result = executeCommand(result);
        }
        else if (nodeName == U"learn")
        {
            loadAiml(std::string(result.c_str()));
            result = "";
        }
        else if (nodeName == U"uppercase")
        {
            toUpper(result);
        }
        else if (nodeName == U"lowercase")
        {
            toLower(result);
        }
        else if (node->first_child().empty())
        {
            if (nodeName == U"star")
            {
                unsigned int index = 0;
                if (node->attribute("index").empty())
                {
                    index = 0;
                }
                else
                {
                    index = node->attribute("index").as_int() - 1;
                }

                // std::cout << "star :" << ((index < capturedTexts.size()) ? capturedTexts[index] : "") << std::endl;
                result = ((index < capturedTexts.size()) ? capturedTexts[index] : "");
            }
            else if (nodeName == U"thatstar")
            {
                unsigned int index = 0;
                if (!node->attribute("index").empty())
                {
                    index = node->attribute("index").as_int() - 1;
                }

                result = index < capturedThatTexts.size() ? capturedThatTexts[index] : "";
            }
            else if (nodeName == U"topicstar")
            {
                unsigned int index = 0;
                if (node->attribute("index").empty())
                {
                    index = 0;
                }
                else
                {
                    index = node->attribute("index").as_int() - 1;
                }
                result = index < capturedTopicTexts.size() ? capturedTopicTexts[index] : "";
            }
            else if (nodeName == U"that")
            {

                tiny_utf8::string indexStr = node->attribute("index").as_string();
                if (!indexStr.find(U',') != -1)
                    indexStr = U"1," + indexStr;
                std::vector<tiny_utf8::string> inds;
                split(inds, indexStr, U",");

                unsigned int index1 = std::stoi(inds[0].c_str()) - 1;
                unsigned int index2 = std::stoi(inds[1].c_str()) - 1;
                result = (index1 < thatList.size()) && (index2 < thatList[index1].size()) ? thatList[index1][index2] : "";
            }
            else if (nodeName == U"sr")
                result = getResponse(capturedTexts.size() ? capturedTexts[0] : "", true);
            else if (nodeName == U"br")
                result = U"\n";
            else if (nodeName == U"get")
                result = parameterValue[node->attribute("name").as_string()];
            else if (nodeName == U"bot")
                result = botVarValue[node->attribute("name").as_string()];
            else if ((nodeName == U"person") || (nodeName == "person2") || (nodeName == "gender"))
                result = capturedTexts.size() ? capturedTexts[0] : "";
            else if (nodeName == U"input")
            {
                unsigned int index = 0;
                if (node->attribute("index").empty())
                {
                    index = 0;
                }
                else
                {
                    index = node->attribute("index").as_int() - 1;
                }
                result = index < inputList.size() ? inputList[index] : "";
            }
            //the following just to avoid warnings !
            else if (nodeName == U"li")
                ;
            else
                logStream << "Warning: unknown tag \"" + nodeName + "\"\n";
        }
        //the following just to avoid warnings !
        else if ((nodeName == "template") || (nodeName == "pattern") || (nodeName == "li")
            || (nodeName == "person") || (nodeName == "person2") || (nodeName == "gender")
            || (nodeName == "parsedCondition"))
            ;
        else
            logStream << "Warning: unknown tag \"" + nodeName + "\"\n";
    }
    normalizeString(result);
    return result;
}
//----------------------------------------
//
//----------------------------------------
//parses a category and creates a correspondant element
void parseCategory(pugi::xml_node* categoryNode)
{
    pugi::xml_node patternNode = categoryNode->first_element_by_path("pattern");
    tiny_utf8::string pattern = resolveNode(&patternNode);
    //std::cout << "pattern :" << pattern << std::endl;
    //replace(pattern,U"*", U" * ");
    normalizeString(pattern);
    //find where to insert the new node
    std::vector<tiny_utf8::string> words;
    split(words, pattern, U" ");
    // for (auto w : words)
    // {
    //     std::cout << "words :" << w << std::endl;
    // }
    Node* whereToInsert = &root;
    for (std::vector<tiny_utf8::string>::const_iterator it = words.begin(); it != words.end(); ++it)
    {
        bool found = false;
        for (auto child : whereToInsert->childs)
        {
            if (child->word == *it)
            {
                whereToInsert = child;
                found = true;
                break;
            }
        }
        if (!found)
        {
            for (; it != words.end(); ++it)
            {
                Node* n = new Node;
                n->word = *it;
                n->parent = whereToInsert;
                int index = 0;
                if (*it == U"*")
                {
                    index = whereToInsert->childs.size();
                }
                else if ((*it != U"_") &&
                    whereToInsert->childs.size() &&
                    (whereToInsert->childs.front()->word == "_"))
                {
                    index = 1;
                }

                std::vector<Node*>::iterator pos = whereToInsert->childs.begin();
                for (int i = 0; i < index; ++i)
                {
                    pos++;
                }
                whereToInsert->childs.emplace(pos, n);
                whereToInsert = n;
            }
            break;
        }
    }

    //Now insert the leaf
    Leaf* leaf = new Leaf;
    leaf->parent = whereToInsert;
    pugi::xml_node thatNode = categoryNode->child("what");
    if (!thatNode.empty())
    {

        leaf->that = thatNode.first_child().text().as_string();
        normalizeString(leaf->that);
    }
    leaf->tmplate = categoryNode->child("template");
    pugi::xml_node parentNode = categoryNode->parent();
    if (!parentNode.empty() && (parentNode.name() == "topic"))
    {
        leaf->topic = parentNode.attribute("name").as_string();
        normalizeString(leaf->topic);
    }
    int index = 0;
    int leafWeight = !leaf->that.empty() + !leaf->topic.empty() * 2;
    for (auto childLeaf : whereToInsert->leafs)
    {
        int childLeafWeight = !childLeaf->that.empty() + !childLeaf->topic.empty() * 2;
        if (leafWeight >= childLeafWeight)
        {
            break;
        }
        index++;
    }

    std::vector<Leaf*>::iterator pos = whereToInsert->leafs.begin();
    for (int i = 0; i < index; ++i)
    {
        pos++;
    }
    whereToInsert->leafs.emplace(pos, leaf);
}

//----------------------------------------
//
//----------------------------------------
bool loadAiml(const std::string& filename)
{
    std::ifstream stream(filename);
    pugi::xml_parse_result result = doc.load(stream);
    if (result)
    {
        std::cout << u8"XML загружен успешно." << std::endl;
    }
    else
    {
        std::cout << u8"Ошибка загрузки XML" << std::endl;
        return false;
    }

    std::vector<pugi::xml_node> categoryList = elementsByTagName(&doc, "category");
    std::cout << u8"Найдено " << categoryList.size() << u8" категорий." << std::endl;
    for (int i = 0; i < categoryList.size(); i++)
    {
        pugi::xml_node n = categoryList[i];
        parseCategory(&n);
    }
    std::cout << u8"Категории обработаны." << std::endl;
    return true;
}
//----------------------------------------
//
//----------------------------------------
//----------------------------------------
//
//----------------------------------------
tiny_utf8::string getResponse(tiny_utf8::string input, const bool& srai)
{
    tiny_utf8::string result("");

    //debug
    if (srai)
    {
        indent++;
    }
    tiny_utf8::string indentSpace = " ";
    logStream << (!srai ? "\n" : "") << indentSpace << (srai ? "::SRAI: " : "::User Input: ") << input << std::endl;
    
    //perform substitutions for input string
    std::vector<tiny_utf8::string>::iterator itOld = subOld.begin();
    std::vector<tiny_utf8::string>::iterator itNew = subNew.begin();
    for (; itOld != subOld.end(); ++itOld, ++itNew)
    {
        replace(input,*itOld, *itNew);
    }
    if (!srai)
    {
        inputList.emplace(inputList.begin(), input);
        if (inputList.size() > MAX_LIST_LENGTH)
        {
            inputList.pop_back();
        }
    }

    std::vector<tiny_utf8::string> capturedTexts;
    std::vector<tiny_utf8::string> capturedThatTexts;
    std::vector<tiny_utf8::string> capturedTopicTexts;

    tiny_utf8::string curTopic = parameterValue["topic"];
    normalizeString(curTopic);
    Leaf* leaf = NULL;

    tiny_utf8::string sentence = input;
    trimString(sentence);
    if (!sentence.empty())
    {
        normalizeString(sentence);
        std::vector<tiny_utf8::string> inputWords;
        split(inputWords, sentence, U" ");

        std::vector<tiny_utf8::string>::const_iterator it = inputWords.begin();

        if (!root.match(it, inputWords, thatList.size() && thatList[0].size() ? thatList[0][0] : "", curTopic, capturedThatTexts, capturedTopicTexts, leaf))
        {
            return U"Ошибка парсинга !";
        }

        Node* parentNode = nullptr;
        if (leaf != nullptr)
        {
            parentNode = leaf->parent;
        }
        tiny_utf8::string matchedPattern;
        if (parentNode != nullptr)
        {
            matchedPattern = parentNode->word;
        }
        bool flag = false;
        if (parentNode != nullptr)
        {
            if (parentNode->parent != nullptr)
            {
                if (parentNode->parent->parent != nullptr)
                {
                    flag = true;
                }
            }
        }
        if (flag)
        {
            while (parentNode->parent->parent)
            {
                parentNode = parentNode->parent;
                matchedPattern = parentNode->word + " " + matchedPattern;
            }
        }

        logStream << indentSpace + "::Matched pattern: [" + matchedPattern + "]" << std::endl;
        if (leaf != nullptr)
        {
            if (!leaf->that.empty())
                logStream << " - Matched that: [" + leaf->that + "]" << std::endl;
            if (!leaf->topic.empty())
                logStream << " - Matched topic: [" + leaf->topic + "]" << std::endl;
            logStream << "\n";
            capturedTexts.clear();
            exactMatch(matchedPattern, sentence, capturedTexts);
            for (auto t : capturedTexts)
            {
                std::cout << "captured text :" << t << std::endl;
            }
            //strip whitespaces from the beggining and the end of result
            if (indent >= MAX_RECURSION)
                result += ("parsesr: Too much recursion (Probable infinite loop)!");
            else
                trimString(result);
            result += " " + resolveNode(&leaf->tmplate, capturedTexts, capturedThatTexts, capturedTopicTexts);

        }
    }


    if (!srai)
    {
        tiny_utf8::string tempResult = result;
        trimString(result);
        //get the sentences of the result splitted by: . ? ! ; and "arabic ?"
        std::vector<tiny_utf8::string> thatSentencesList;
        split(thatSentencesList, tempResult, ".");
        std::vector<tiny_utf8::string> inversedList;
        for (std::vector<tiny_utf8::string>::iterator it = thatSentencesList.begin(); it != thatSentencesList.end(); ++it)
        {
            //perform substitutions for that string
            std::vector<tiny_utf8::string>::iterator itOld = subOld.begin();
            std::vector<tiny_utf8::string>::iterator itNew = subNew.begin();
            for (; itOld != subOld.end(); ++itOld, ++itNew)
            {
                replace(tempResult, *itOld, *itNew);
            }
            normalizeString(*it);
            inversedList.emplace(inversedList.begin(), *it);
        }
        thatList.emplace(thatList.begin(), inversedList.begin(), inversedList.end());

        if (thatList.size() > MAX_LIST_LENGTH)
        {
            thatList.pop_back();
        }
    }
    //debug
    logStream << indentSpace + "::Result: " + result + "\n";
    if (srai)
        indent--;
    std::cout << "logStream: " << logStream.str() << std::endl;
    return result;
}
//----------------------------------------
//
//----------------------------------------
void main(void)
{
    setlocale(LC_ALL, "ru_RU.UTF8");
    std::locale::global(std::locale("ru_RU.UTF8"));

    /*
    tiny_utf8::string regExp=U"привет *";
    tiny_utf8::string str=U"привет чувак";
    std::vector<tiny_utf8::string> capturedText;
    bool matched = exactMatch(regExp, str, capturedText);
    std::cout << u8"Захваченный текст" << std::endl;
    for (auto txt : capturedText)
    {
        std::cout << txt << std::endl;
    }
    */

    
    init();
    loadAiml("aiml/test.aiml");
    tiny_utf8::string input = U"Привет чел ыы бум";
    //tiny_utf8::string input = U"DO YOU FIND ME ATTRACTIVE";
    //tiny_utf8::string input = U"HI";
    //tiny_utf8::string input = U"Меня зовут Андрей";
    tiny_utf8::string res = getResponse(input);
    std::cout << "res: " << res;

    // input = U"Пока";
    // res = getResponse(input);
    // std::cout << "res: " << res;

    clear();
    
}