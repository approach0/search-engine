#ifndef CPPJIEBA_POS_TAGGING_H
#define CPPJIEBA_POS_TAGGING_H

#include "MixSegment.hpp"
#include "limonp/StringUtil.hpp"
#include "DictTrie.hpp"

namespace cppjieba {
using namespace limonp;

static const char* const POS_M = "m";
static const char* const POS_ENG = "eng";
static const char* const POS_X = "x";

class PosTagger {
 public:
  PosTagger(const string& dictPath,
    const string& hmmFilePath,
    const string& userDictPath = "")
    : segment_(dictPath, hmmFilePath, userDictPath) {
  }
  PosTagger(const DictTrie* dictTrie, const HMMModel* model) 
    : segment_(dictTrie, model) {
  }
  ~PosTagger() {
  }

  bool Tag(const string& src, vector<pair<string, string> >& res) const {
    vector<string> CutRes;
    segment_.Cut(src, CutRes);

    for (vector<string>::iterator itr = CutRes.begin(); itr != CutRes.end(); ++itr) {
        res.push_back(make_pair(*itr, LookupTag(*itr)));
    }
    return !res.empty();
  }

	string LookupTag(const string &str) const {
		const DictTrie * dict = segment_.GetDictTrie();
		RuneStrArray runes;
		const DictUnit *tmp = NULL;
 
		assert(dict != NULL);

		if (!DecodeRunesInString(str, runes)) {
			XLOG(ERROR) << "Decode failed.";
			return string(POS_X);
		}

		tmp = dict->Find(runes.begin(), runes.end());

		if (tmp == NULL || tmp->tag.empty())
			return SpecialRule(runes);
		else
			return tmp->tag;
	}

 private:
  const char* SpecialRule(const RuneStrArray& unicode) const {
    size_t m = 0;
    size_t eng = 0;
    for (size_t i = 0; i < unicode.size() && eng < unicode.size() / 2; i++) {
      if (unicode[i].rune < 0x80) {
        eng ++;
        if ('0' <= unicode[i].rune && unicode[i].rune <= '9') {
          m++;
        }
      }
    }
    // ascii char is not found
    if (eng == 0) {
      return POS_X;
    }
    // all the ascii is number char
    if (m == eng) {
      return POS_M;
    }
    // the ascii chars contain english letter
    return POS_ENG;
  }

  MixSegment segment_;
}; // class PosTagger

} // namespace cppjieba

#endif
