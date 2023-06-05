#include <pch.hpp>

// poisoned malloc •﹏• (all the functions used malloc & free MUST be defined here so that compilation error won't happen)
void c_str_insert(const unsigned char* destination, int pos, const unsigned char* src) { // From Stackoverflow
    unsigned char* strC;
    strC = (unsigned char*)xmalloc(strlen((char*)destination)+strlen((char*)src)+1);
    strncpy((char*)strC,(char*)destination,pos);
    strC[pos] = '\0';
    strcat((char*)strC,(char*)src);
    strcat((char*)strC,(char*)destination+pos);
    strcpy((char*)destination,(char*)strC);
    free(strC);
}
bool c_str_prefix(const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}
void write_to_file(const char* path, const char* fmt, auto&&... Args) {
    FILE* file = fopen(path, "a"); assert(file != NULL);
    fprintf(file, fmt, std::forward<decltype(Args)>(Args)...);
    fclose(file);
}
void delete_file(const char* path) {
    remove(path);
}
std::string exec(const char* cmd) { // From Stackoverflow
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}
std::string get_self_path() { // From Stackoverflow
#ifdef _WIN32
    wchar_t path[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return path;
#else
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::string(result, (count > 0) ? count : 0);
#endif
}
void replace_all(std::string& str, const std::string& from, const std::string& to) { // From Stackoverflow
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

subhook::Hook TheHook, HookToCxxInit, HookToCppReadMainFile;//, SetGlobalFriendHook;