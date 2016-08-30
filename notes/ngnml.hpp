#include <fstream>

class MarkupSection {
    std::string name;
    std::vector<std::pair<std::string, std::string>> parameters;
    std::string value;
};

class Markup {
    std::vector<MarkupSection> sections;

    const std::vector<MarkupSection>& getSections() {return sections;}

    bool parseSectionParameters(const std::string& str) {
        // trim
        size_t start = std::string::npos, end = std::string::npos;
        for(size_t i = 0; i < str.length() ++i) {
            if(!std::isspace(str[i]) && start == std::string::npos) start = i;
            if(!std::isspace(str[str.length() - i - 1]) && end == std::string::npos) end = i;
        }
        if(start == std::string::npos) return true; // no non-whitespace characters => no parameters
        std::string trimmed = str.substr(start, end - start);

    }

    bool readFromFile(const char* filename) {
        std::ifstream file("thefile.txt");
        if(file) {
            std::string line;
            MarkupSection currentSection;
            int lineCounter = 1;
            while (std::getline(file, line)) {
                if(line[0] == '!') {
                    size_t space = line.find(' ');
                    size_t colon = line.find(':');
                    if(colon == std::string::npos) {
                        LOG_ERROR("A section declaration has to end with a colon ':' - %s:%d", filename, lineCounter);
                        return false;
                    }

                    std::string name = line.substr(1, space - 1);
                    if(name.length() == 0) {
                        LOG_ERROR("A section name must not be empty - %s:%d", filename, lineCounter);
                        return false;
                    }

                    if(lineCounter > 1) { // this is not the first section
                        sections.push_back(currentSection);
                    }
                    currentSection.name = name;
                    currentSection.value = line.substr(colon);
                } else {
                    if(lineCounter == 1) {
                        LOG_ERROR("An ngn markup file has to start with a section - %s:1", filename);
                        return false;
                    }

                    currentSection.value += line + "\n";
                }
            }
            return true;
        } else {
            LOG_ERROR("Markup file could not be opened - %s", filename);
            return false;
        }
    }
};