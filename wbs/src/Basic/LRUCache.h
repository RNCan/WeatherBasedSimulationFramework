/* 
 * File:   lrucache.hpp
 * Author: Alexander Ponomarev
 *
 * Created on June 20, 2013, 5:09 PM
 */

#ifndef _LRUCACHE_HPP_INCLUDED_
#define	_LRUCACHE_HPP_INCLUDED_

#include <unordered_map>
#include <list>
#include <cstddef>
#include <stdexcept>

namespace cache {

template<typename key_t, typename value_t>
class lru_cache {
public:
	typedef typename std::pair<key_t, value_t> key_value_pair_t;
	typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

	lru_cache(size_t max_size) :
		_max_size(max_size) 
	{}
	
	void put(const key_t& key, const value_t& value) 
	{
		auto it = _cache_items_map.find(key);
		if (it != _cache_items_map.end()) {
			_cache_items_list.erase(it->second);
			_cache_items_map.erase(it);
		}
			
		_cache_items_list.push_front(key_value_pair_t(key, value));
		_cache_items_map[key] = _cache_items_list.begin();
		
		if (_cache_items_map.size() > _max_size) {
			auto last = _cache_items_list.end();
			last--;
			_cache_items_map.erase(last->first);
			_cache_items_list.pop_back();
		}
	}
	
	/*const*/ value_t& get(const key_t& key)
	{
		auto it = _cache_items_map.find(key);
		if (it == _cache_items_map.end()) {
			throw std::range_error("There is no such key in cache");
		} else {
			_cache_items_list.splice(_cache_items_list.begin(), _cache_items_list, it->second);
			return it->second->second;
		}
	}
	
	bool exists(const key_t& key) const 
	{
		return _cache_items_map.find(key) != _cache_items_map.end();
	}
	
	size_t size() const {	return _cache_items_map.size(); }
	
	void clear() {_cache_items_list.clear();_cache_items_map.clear();}
private:
	std::list<key_value_pair_t> _cache_items_list;
	std::unordered_map<key_t, list_iterator_t> _cache_items_map;
	size_t _max_size;
};

} // namespace lru

#endif	/* _LRUCACHE_HPP_INCLUDED_ */


#if 0







/******************************************************************************/
/*  Copyright (c) 2010-2011, Tim Day <timday@timday.com>                      */
/*                                                                            */
/*  Permission to use, copy, modify, and/or distribute this software for any  */
/*  purpose with or without fee is hereby granted, provided that the above    */
/*  copyright notice and this permission notice appear in all copies.         */
/*                                                                            */
/*  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES  */
/*  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF          */
/*  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR   */
/*  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN     */
/*  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF   */
/*  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.            */
/******************************************************************************/

#ifndef _lru_cache_using_boost_
#define _lru_cache_using_boost_

#include <boost/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/function.hpp>  
#include <cassert>

// Class providing fixed-size (by number of records)
// LRU-replacement cache of a function with signature
// V f(K).
// SET is expected to be one of boost::bimaps::set_of
// or boost::bimaps::unordered_set_of
template <
  typename K,
  typename V,
  template <typename...> class SET
  > class lru_cache_using_boost
{
 public:

  typedef K key_type;
  typedef V value_type;
  
  typedef boost::bimaps::bimap<
    SET<key_type>,
    boost::bimaps::list_of<value_type>
    > container_type;

  // Constuctor specifies the cached function and 
  // the maximum number of records to be stored.  
  lru_cache_using_boost(
    const boost::function<value_type(const key_type&)>& f,
    size_t c
  )
    :_fn(f)
    ,_capacity(c)
  {
    assert(_capacity!=0);
  }
  
  // Obtain value of the cached function for k
  value_type operator()(const key_type& k) {

    // Attempt to find existing record
    const typename container_type::left_iterator it
      =_container.left.find(k);

    if (it==_container.left.end()) {

      // We don't have it:

      // Evaluate function and create new record
      const value_type v=_fn(k);
      insert(k,v);

      // Return the freshly computed value
      return v;

    } else {
      
      // We do have it:

      // Update the access record view.
      _container.right.relocate(
	_container.right.end(),
	_container.project_right(it)
      );
      
      // Return the retrieved value
      return it->second;
    }
  }
  
  // Obtain the cached keys, most recently used element
  // at head, least recently used at tail.
  // This method is provided purely to support testing.
  template <typename IT> void get_keys(IT dst) const {
    typename container_type::right_const_reverse_iterator src
	=_container.right.rbegin();
    while (src!=_container.right.rend()) {
      *dst++=(*src++).second;
    }
  }

 private:
  
  void insert(const key_type& k,const value_type& v) {

    assert(_container.size()<=_capacity);
    
    // If necessary, make space
    if (_container.size()==_capacity) {
      // by purging the least-recently-used element
      _container.right.erase(_container.right.begin());
    }
    
    // Create a new record from the key and the value
    // bimap's list_view defaults to inserting this at
    // the list tail (considered most-recently-used).
    _container.insert(
      typename container_type::value_type(k,v)
    );
  }
  
  const boost::function<value_type(const key_type&)> _fn;
  const size_t _capacity;
  container_type _container;
};

#endif

#endif