#include "indri/Repository.hpp"
#include "indri/Parameters.hpp"
#include "indri/ParsedDocument.hpp"
#include "indri/CompressedCollection.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include "list/list.h"
#include "tree/treap.h"
#include "invlist/invlist.h"

#ifdef __cplusplus
}
#endif

struct term_index {
	indri::collection::Repository  repo;
	indri::api::Parameters         parameters;
	indri::api::ParsedDocument     document;
	indri::index::Index           *index;
	uint32_t                       avgDocLen;
	std::vector<char*>             save;

	codec_buf_struct_info_t *cinfo;
	struct treap_node *trp_root;
	size_t memo_usage; /* treap and inverted list only */
};
