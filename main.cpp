#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <functional>
#include <stack>


class XML_node {
public:
    std::string tag;
    std::string value;
    std::vector<std::unique_ptr<XML_node>> children;
    XML_node *parent;

    XML_node(const std::string &tag, const std::string &value) : tag(tag), value(value) {}

    void append(std::unique_ptr<XML_node> child) {
        child->parent = this;
        children.push_back(std::move(child));
    }

    std::string stringify(const int depth = 0) const {
        const std::string indent = std::string(depth * 2, ' ');
        std::string result = "";

        result += indent + "<" + tag + ">" + value;
        if (children.size()) {
            result += "\n";
        }
        for (const auto &child: children) {
            result += child->stringify(depth + 1);
        }
        if (children.size()) {
            result += indent;
        }
        result += "</" + tag + ">" + "\n";

        return result;
    };

    std::string stringify_element(const int depth = 0) const {
        const std::string indent = std::string(depth * 2, ' ');
        std::string result = "";

        result += indent + "<" + tag + ">" + value;
        result += "</" + tag + ">" + "\n";

        return result;
    };

    void for_each(std::function<void(const XML_node &)> callback) {
        callback(*this);
        for (const auto &child: children) {
            child->for_each(callback);
        };
    };

    void print() const {
        std::string xml = stringify();
        std::cout << xml << std::endl;
    }

    void print_element() const {
        std::string xml = stringify_element();
        std::cout << xml << std::endl;
    }
};

class XML_document {
public:
    XML_document() : root_node(nullptr) {};

    XML_node *getRootNode() {
        return root_node.get();
    }

    void parse(const std::string &xml) {
        int pos = 0;
        root_node = parse_node(xml, pos);
    };

    void load(const std::string &path) {
        const std::string xml = read_file(path);
        parse(xml);
    };

    void save(const std::string &path) {
        std::string xml = stringify();
        write_file(path, xml);
    };

    void print() {
        std::string xml = stringify();
        std::cout << xml << std::endl;
    };

    void print_element() {
        std::string xml = stringify_element();
        std::cout << xml << std::endl;
    };


    void for_each(std::function<void(const XML_node &)> callback) {
        root_node->for_each(callback);
    };

private:
    std::unique_ptr<XML_node> root_node;

    std::unique_ptr<XML_node> parse_node(const std::string &xml, int &pos) {
        std::string tag = get_next_tag(xml, pos);
        std::string value = get_next_value(xml, pos);
        std::unique_ptr<XML_node> node(new XML_node(tag, value));

        std::string next_tag = get_next_tag(xml, pos);
        while (next_tag != ("/" + tag) && pos < xml.size()) {
            pos -= next_tag.size() + 2;
            node->append(parse_node(xml, pos));
            next_tag = get_next_tag(xml, pos);
        }

        return node;
    };

    std::string get_next_tag(const std::string &xml, int &pos) {
        pos = xml.find("<", pos) + 1;
        int end = xml.find(">", pos);
        std::string tag = xml.substr(pos, end - pos);
        pos = end + 1;
        return tag;
    };

    std::string get_next_value(const std::string &xml, int &pos) {
        int start = pos;
        pos = xml.find("<", pos);
        return trim(xml.substr(start, pos - start));
    };

    std::string read_file(const std::string &path) {
        std::ifstream file(path);

        if (!file) {
            throw std::runtime_error("File not found");
        }

        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    };

    void write_file(const std::string &path, const std::string &content) {
        std::ofstream file(path);

        if (!file) {
            throw std::runtime_error("File not found");
        }

        file << content;
    };

    std::string stringify() {
        return root_node->stringify();
    };

    std::string stringify_element() {
        return root_node->stringify_element();
    };


    std::string trim(const std::string &str) {
        std::string result = "";
        int start_pos = 0;
        int end_pos = str.length() - 1;


        for (; start_pos < str.length(); start_pos += 1) {
            char ch = str[start_pos];

            if (isspace(ch)) {
                continue;
            } else {
                break;
            }
        };
        for (; end_pos >= 0; end_pos -= 1) {
            char ch = str[end_pos];

            if (isspace(ch)) {
                continue;
            } else {
                break;
            }
        };

        return str.substr(start_pos, end_pos - start_pos + 1);
    };
};

class Iterator {
public:
    Iterator(XML_node *node) : current(node) {
        if (current) {
            stack.push(current);
        }
    }

    Iterator &operator++() {
        if (!stack.empty()) {
            current = stack.top();
            stack.pop();
            for (auto it = current->children.rbegin(); it != current->children.rend(); ++it) {
                stack.push(it->get());
            }
        } else {
            current = nullptr;
        }
        return *this;
    }

    bool operator==(const Iterator &other) const {
        return current == other.current;
    }

    bool operator!=(const Iterator &other) const {
        return !(*this == other);
    }

    XML_node &operator*() {
        return *current;
    }

    XML_node *operator->() {
        return current;
    }

    void append(std::unique_ptr<XML_node> &&uniquePtr) const {
        if (current) {
            current->children.push_back(std::move(uniquePtr));
        }
    }


private:
    XML_node *current;
    std::stack<XML_node *> stack;
};

class resurs_XML {
public:
    Iterator begin() {
        return Iterator(document.getRootNode());
    }

    Iterator end() {
        return Iterator(nullptr);
    }

    void parse(const std::string &xml) {
        document.parse(xml);
    }

    void load(const std::string &path) {
        document.load(path);
    }

    void save(const std::string &path) {
        document.save(path);
    }

    bool Erase(Iterator it) {
        if (it == end()) {
            return false;
        }
        Iterator parentIt = it->parent;
        if (parentIt == end()) {
            return false;
        }
        parentIt->children.insert(parentIt->children.end(), std::make_move_iterator(it->children.begin()),
                                  std::make_move_iterator(it->children.end()));

        parentIt->children.erase(std::remove_if(parentIt->children.begin(), parentIt->children.end(),
                                                [&it](const std::unique_ptr<XML_node> &element) {
                                                    return element.get() == &(*it);
                                                }), parentIt->children.end());

        return true;
    }

    void print() {
        document.print();
    }

    void print_element() {
        document.print_element();
    }

    void for_each(std::function<void(const XML_node &)> callback) {
        document.for_each(callback);
    }

    static std::unique_ptr<resurs_XML> create() {
        return std::unique_ptr<resurs_XML>();
    }

    static std::unique_ptr<resurs_XML> create(const std::string &filePath) {
        std::unique_ptr<resurs_XML> instance(new resurs_XML());
        instance->load(filePath);
        return instance;
    }

    Iterator add(const std::string &tag, const std::string &value, const Iterator &ItParent) {
        if (ItParent == end()) {
            return end();
        }
        std::unique_ptr<XML_node> new_node = std::make_unique<XML_node>(tag, value);

        ItParent.append(std::move(new_node));
        auto ItAddElement = find(tag, value);
        return ItAddElement;
    }

    Iterator find(const std::string &tag, const std::string &value) {
        for (auto it = begin(); it != end(); ++it) {
            if ((*it).tag == tag && (*it).value == value) {
                return it;
            }
        }
        return end();
    }

private:
    XML_document document;
};

int main() {
    std::unique_ptr<resurs_XML> resource = resurs_XML::create("./example.txt");

    std::cout << "All elements:" << std::endl;
    resource->print();

    std::string searchTag = "title2";
    std::string searchValue = "1984";
    Iterator foundIt = resource->find(searchTag, searchValue);

    if (foundIt != resource->end()) {
        std::cout << "\nFound element:" << std::endl;
        foundIt->print_element();
    } else {
        std::cout << "\nElement not found." << std::endl;
    }

    std::string newTag = "newBook";
    std::string newValue = "New Book Title";
    Iterator parentIt = resource->find("library", "Bibli");

    if (parentIt != resource->end()) {
        Iterator addedIt = resource->add(newTag, newValue, parentIt);
        if (addedIt != resource->end()) {
            std::cout << "\nAdd element:" << std::endl;
            addedIt->print_element();
        } else {
            std::cout << "\nFailed to add the new element." << std::endl;
        }
    } else {
        std::cout << "\nParent element not found." << std::endl;
    }
    resource->print();

    if (foundIt != resource->end()) {
        if (resource->Erase(foundIt)) {
            std::cout << "\nElement has been erased." << std::endl;
        } else {
            std::cout << "\nFailed to erase the element." << std::endl;
        }
    }
    resource->save("./output.txt");
    std::cout << "\nWork completed :)" << std::endl;
    return 0;
}
