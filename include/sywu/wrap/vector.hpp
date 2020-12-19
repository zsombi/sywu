#ifndef SYWU_VECTOR_HPP
#define SYWU_VECTOR_HPP

namespace sywu
{

#include <vector>
using std::vector;

/// Vector utility, loops a \a predicate through a \a vector.
template <typename Type, typename Allocator, typename Predicate>
void for_each(vector<Type, Allocator>& vector, const Predicate& predicate)
{
    for_each(vector.begin(), vector.end(), predicate);
}
template <typename Type, typename Allocator, typename Predicate>
void for_each(const vector<Type, Allocator>& vector, const Predicate& predicate)
{
    for_each(vector.begin(), vector.end(), predicate);
}

/// Vector utility, loops a \a predicate through a \a vector.
template <typename Type, typename Allocator, typename Predicate>
decltype(auto) find_if(vector<Type, Allocator>& vector, const Predicate& predicate)
{
    return find_if(vector.begin(), vector.end(), predicate);
}
template <typename Type, typename Allocator, typename Predicate>
decltype(auto) find_if(const vector<Type, Allocator>& vector, const Predicate& predicate)
{
    return find_if(vector.begin(), vector.end(), predicate);
}

/// Vector utility, removes the occurences of \a value from a \a vector.
template <typename Type, typename Allocator, typename VType>
void erase(vector<Type, Allocator>& vector, const VType& value)
{
    vector.erase(remove(vector.begin(), vector.end(), value), vector.end());
}

/// Vector utility, removes the first occurence of \a value from a \a vector.
template <typename Type, typename Allocator, typename VType>
void erase_first(vector<Type, Allocator>& vector, const VType& value)
{
    auto it = find(vector.begin(), vector.end(), value);
    if (it != vector.end())
    {
        vector.erase(it);
    }
}

/// Vector utility, removes the occurences for which the predicate gives affirmative result.
template <typename Type, typename Allocator, typename Predicate>
void erase_if(vector<Type, Allocator>& v, const Predicate& predicate)
{
    v.erase(remove_if(v.begin(), v.end(), predicate), v.end());
}

} // namespace sywu

#endif // SYWU_VECTOR_HPP
