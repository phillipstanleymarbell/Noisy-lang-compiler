/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: noisy.proto */

#ifndef PROTOBUF_C_noisy_2eproto__INCLUDED
#define PROTOBUF_C_noisy_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1000000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1001000 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _Noisy__NoisySourceInfo Noisy__NoisySourceInfo;
typedef struct _Noisy__NoisyIrNode Noisy__NoisyIrNode;
typedef struct _Noisy__NoisyToken Noisy__NoisyToken;
typedef struct _Noisy__NoisyScope Noisy__NoisyScope;
typedef struct _Noisy__NoisySymbol Noisy__NoisySymbol;
typedef struct _Noisy__NoisyState Noisy__NoisyState;


/* --- enums --- */


/* --- messages --- */

struct  _Noisy__NoisySourceInfo
{
  ProtobufCMessage base;
  /*
   *	Not yet used; for when we implement includes, this will be
   *	the 'genealogy' of includes leading to this token.
   */
  size_t n_genealogy;
  char **genealogy;
  char *filename;
  int64_t linenumber;
  int64_t columnnumber;
  int64_t length;
};
#define NOISY__NOISY_SOURCE_INFO__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&noisy__noisy_source_info__descriptor) \
    , 0,NULL, NULL, 0, 0, 0 }


struct  _Noisy__NoisyIrNode
{
  ProtobufCMessage base;
  int64_t type;
  /*
   *	Syntactic (AST) information.
   */
  char *tokenstring;
  Noisy__NoisySourceInfo *sourceinfo;
  Noisy__NoisySymbol *symbol;
  /*
   *	Used for coloring the IR tree, e.g., during Graphviz/dot
   *	generation.
   */
  int64_t nodecolor;
};
#define NOISY__NOISY_IR_NODE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&noisy__noisy_ir_node__descriptor) \
    , 0, NULL, NULL, NULL, 0 }


struct  _Noisy__NoisyToken
{
  ProtobufCMessage base;
  int64_t type;
  char *identifier;
  protobuf_c_boolean has_integerconst;
  int64_t integerconst;
  protobuf_c_boolean has_realconst;
  double realconst;
  char *stringconst;
  Noisy__NoisySourceInfo *sourceinfo;
};
#define NOISY__NOISY_TOKEN__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&noisy__noisy_token__descriptor) \
    , 0, NULL, 0,0, 0,0, NULL, NULL }


struct  _Noisy__NoisyScope
{
  ProtobufCMessage base;
  /*
   *	For named scopes (at the moment, only Progtypes).
   */
  char *identifier;
  /*
   *	Hierarchy. The firstChild is used to access its siblings
   *	via firstChild->next.
   */
  size_t n_childscopes;
  Noisy__NoisyScope **childscopes;
  /*
   *	Symbols in this scope. The list of symbols is accesed
   *	via firstSymbol->next.
   */
  size_t n_symbols;
  Noisy__NoisySymbol **symbols;
  /*
   *	Where in source scope begins and ends
   */
  Noisy__NoisySourceInfo *begin;
  Noisy__NoisySourceInfo *end;
};
#define NOISY__NOISY_SCOPE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&noisy__noisy_scope__descriptor) \
    , NULL, 0,NULL, 0,NULL, NULL, NULL }


struct  _Noisy__NoisySymbol
{
  ProtobufCMessage base;
  char *identifier;
  /*
   *	This field is duplicated in the AST node, since only
   *	identifiers get into the symbol table:
   */
  Noisy__NoisySourceInfo *sourceinfo;
  /*
   *	Declaration, type definition, use, etc. (kNoisySymbolTypeXXX)
   */
  int64_t symboltype;
  /*
   *	Scope within which sym appears.
   */
  Noisy__NoisyScope *scope;
  /*
   *	If an identifier use, definition's Sym, if any.
   */
  Noisy__NoisySymbol *definition;
  /*
   *	Subtree in AST that represents typeexpr.
   */
  Noisy__NoisyIrNode *typetree;
  /*
   *	If an I_CONST, its value.
   */
  protobuf_c_boolean has_intconst;
  int64_t intconst;
  protobuf_c_boolean has_realconst;
  double realconst;
  char *stringconst;
};
#define NOISY__NOISY_SYMBOL__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&noisy__noisy_symbol__descriptor) \
    , NULL, NULL, 0, NULL, NULL, NULL, 0,0, 0,0, NULL }


struct  _Noisy__NoisyState
{
  ProtobufCMessage base;
  size_t n_noisyirroot;
  Noisy__NoisyIrNode **noisyirroot;
  size_t n_noisyirtopscope;
  Noisy__NoisyScope **noisyirtopscope;
};
#define NOISY__NOISY_STATE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&noisy__noisy_state__descriptor) \
    , 0,NULL, 0,NULL }


/* Noisy__NoisySourceInfo methods */
void   noisy__noisy_source_info__init
                     (Noisy__NoisySourceInfo         *message);
size_t noisy__noisy_source_info__get_packed_size
                     (const Noisy__NoisySourceInfo   *message);
size_t noisy__noisy_source_info__pack
                     (const Noisy__NoisySourceInfo   *message,
                      uint8_t             *out);
size_t noisy__noisy_source_info__pack_to_buffer
                     (const Noisy__NoisySourceInfo   *message,
                      ProtobufCBuffer     *buffer);
Noisy__NoisySourceInfo *
       noisy__noisy_source_info__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   noisy__noisy_source_info__free_unpacked
                     (Noisy__NoisySourceInfo *message,
                      ProtobufCAllocator *allocator);
/* Noisy__NoisyIrNode methods */
void   noisy__noisy_ir_node__init
                     (Noisy__NoisyIrNode         *message);
size_t noisy__noisy_ir_node__get_packed_size
                     (const Noisy__NoisyIrNode   *message);
size_t noisy__noisy_ir_node__pack
                     (const Noisy__NoisyIrNode   *message,
                      uint8_t             *out);
size_t noisy__noisy_ir_node__pack_to_buffer
                     (const Noisy__NoisyIrNode   *message,
                      ProtobufCBuffer     *buffer);
Noisy__NoisyIrNode *
       noisy__noisy_ir_node__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   noisy__noisy_ir_node__free_unpacked
                     (Noisy__NoisyIrNode *message,
                      ProtobufCAllocator *allocator);
/* Noisy__NoisyToken methods */
void   noisy__noisy_token__init
                     (Noisy__NoisyToken         *message);
size_t noisy__noisy_token__get_packed_size
                     (const Noisy__NoisyToken   *message);
size_t noisy__noisy_token__pack
                     (const Noisy__NoisyToken   *message,
                      uint8_t             *out);
size_t noisy__noisy_token__pack_to_buffer
                     (const Noisy__NoisyToken   *message,
                      ProtobufCBuffer     *buffer);
Noisy__NoisyToken *
       noisy__noisy_token__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   noisy__noisy_token__free_unpacked
                     (Noisy__NoisyToken *message,
                      ProtobufCAllocator *allocator);
/* Noisy__NoisyScope methods */
void   noisy__noisy_scope__init
                     (Noisy__NoisyScope         *message);
size_t noisy__noisy_scope__get_packed_size
                     (const Noisy__NoisyScope   *message);
size_t noisy__noisy_scope__pack
                     (const Noisy__NoisyScope   *message,
                      uint8_t             *out);
size_t noisy__noisy_scope__pack_to_buffer
                     (const Noisy__NoisyScope   *message,
                      ProtobufCBuffer     *buffer);
Noisy__NoisyScope *
       noisy__noisy_scope__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   noisy__noisy_scope__free_unpacked
                     (Noisy__NoisyScope *message,
                      ProtobufCAllocator *allocator);
/* Noisy__NoisySymbol methods */
void   noisy__noisy_symbol__init
                     (Noisy__NoisySymbol         *message);
size_t noisy__noisy_symbol__get_packed_size
                     (const Noisy__NoisySymbol   *message);
size_t noisy__noisy_symbol__pack
                     (const Noisy__NoisySymbol   *message,
                      uint8_t             *out);
size_t noisy__noisy_symbol__pack_to_buffer
                     (const Noisy__NoisySymbol   *message,
                      ProtobufCBuffer     *buffer);
Noisy__NoisySymbol *
       noisy__noisy_symbol__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   noisy__noisy_symbol__free_unpacked
                     (Noisy__NoisySymbol *message,
                      ProtobufCAllocator *allocator);
/* Noisy__NoisyState methods */
void   noisy__noisy_state__init
                     (Noisy__NoisyState         *message);
size_t noisy__noisy_state__get_packed_size
                     (const Noisy__NoisyState   *message);
size_t noisy__noisy_state__pack
                     (const Noisy__NoisyState   *message,
                      uint8_t             *out);
size_t noisy__noisy_state__pack_to_buffer
                     (const Noisy__NoisyState   *message,
                      ProtobufCBuffer     *buffer);
Noisy__NoisyState *
       noisy__noisy_state__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   noisy__noisy_state__free_unpacked
                     (Noisy__NoisyState *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Noisy__NoisySourceInfo_Closure)
                 (const Noisy__NoisySourceInfo *message,
                  void *closure_data);
typedef void (*Noisy__NoisyIrNode_Closure)
                 (const Noisy__NoisyIrNode *message,
                  void *closure_data);
typedef void (*Noisy__NoisyToken_Closure)
                 (const Noisy__NoisyToken *message,
                  void *closure_data);
typedef void (*Noisy__NoisyScope_Closure)
                 (const Noisy__NoisyScope *message,
                  void *closure_data);
typedef void (*Noisy__NoisySymbol_Closure)
                 (const Noisy__NoisySymbol *message,
                  void *closure_data);
typedef void (*Noisy__NoisyState_Closure)
                 (const Noisy__NoisyState *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor noisy__noisy_source_info__descriptor;
extern const ProtobufCMessageDescriptor noisy__noisy_ir_node__descriptor;
extern const ProtobufCMessageDescriptor noisy__noisy_token__descriptor;
extern const ProtobufCMessageDescriptor noisy__noisy_scope__descriptor;
extern const ProtobufCMessageDescriptor noisy__noisy_symbol__descriptor;
extern const ProtobufCMessageDescriptor noisy__noisy_state__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_noisy_2eproto__INCLUDED */
