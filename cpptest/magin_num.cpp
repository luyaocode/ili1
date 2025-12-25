#include <cstdio>
#include <cstdint>

template<class...> struct tp{};

consteval std::uint64_t summon()
{
    constexpr auto src = tp<char,char,char,char,char,char,char,char,char,char,char,char,char,char>{};
    std::uint64_t key = 0;
    auto push = [&](auto x){ key = (key << 5) - key + x; };
    [&]<std::size_t...I>(std::index_sequence<I...>){
        ((push(sizeof(tp<char[I+1]>) - sizeof(tp<char[I]>))),...);
    }(std::make_index_sequence<sizeof(src)>{});
    return key;
}

int main()
{
    alignas(16) static constexpr char sig[]{
        char(summon()>>56),  char(summon()>>48),
        char(summon()>>40),  char(summon()>>32),
        char(summon()>>24),  char(summon()>>16),
        char(summon()>> 8),  char(summon()>> 0),
        0
    };
    std::printf(&sig[sizeof(sig)-sizeof(sig[0])-sizeof(sig[0])]);
}
