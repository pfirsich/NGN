#include <cstdio>
#include <cstring>

class fakestring {
private:
    char* data;
public:
    fakestring(const char* str) {
        data = new char[strlen(str)+1];
        strcpy(data, str);
        printf("constructor");
    }

    ~fakestring() {
        delete[] data;
        printf("destructor");
    }

    const char* c_str() const {
        return data;
    }
};

void print(const char* str) {
    puts(str);
}

fakestring giveString() {
    fakestring ret("This is a generated string.");
    return ret;
}

int main(int argc, char** argv) {
    print(giveString().c_str());
}