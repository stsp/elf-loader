#ifndef VDL_FILE_H
#define VDL_FILE_H

#include <stdint.h>
#include <sys/types.h>
#include <link.h>

struct VdlContext;
struct VdlList;

enum VdlFileLookupType
{
  // indicates that lookups within this object should be performed
  // using the global scope only and that local scope should be ignored.
  FILE_LOOKUP_GLOBAL_ONLY,
  FILE_LOOKUP_GLOBAL_LOCAL,
  FILE_LOOKUP_LOCAL_GLOBAL,
  FILE_LOOKUP_LOCAL_ONLY,
};


// the file_ prefix indicates that this variable identifies
// a file offset from the start of the file.
// the mem_ prefix indicates that this variable indentifies
// a pointer in memory
// the _align postfix indicates that this variable identifies
// a variable aligned to the underlying aligning constraint.
struct VdlFileMap
{
  unsigned long file_start_align;
  unsigned long file_size_align;
  // mem_start_align is the memory equivalent of file_start_align
  unsigned long mem_start_align;
  // mem_size_align is the memory equivalent of file_size_align
  unsigned long mem_size_align;
  // mem_zero_start locates the start of a zero-memset area.
  unsigned long mem_zero_start;
  unsigned long mem_zero_size;
  // mem_anon_start_align locates the start of a set of 
  // zero-initialized anon pages.
  unsigned long mem_anon_start_align;
  unsigned long mem_anon_size_align;
};


struct VdlFile
{
  // The following fields are part of the ABI. Don't change them
  unsigned long load_base;
  // the fullname of this file.
  char *filename;
  // pointer to the PT_DYNAMIC area
  unsigned long dynamic;
  struct VdlFile *next;
  struct VdlFile *prev;

  // The following fields are theoretically not part of the ABI but
  // some pieces of the libc code do use some of them so we have to be careful

  // this field is here just for padding to allow l_ns to be located at the
  // right offset.
  uint8_t l_real[sizeof(void*)];

  // This field (named l_ns in the libc elf loader)
  // is used by the libc to determine whether malloc
  // is called from within the main namespace (value is zero)
  // or another namespace (value is not zero). If malloc is called
  // from the main namespace, it uses brk to allocate address space
  // from the OS. If it is called from another namespace, it uses
  // mmap to allocate address space to make sure that the malloc
  // from the main namespace is not confused.
  // theoretically, this field is an index to the right namespace
  // but since it is used only to determine whether this object
  // is located in the main namespace or not, we just set it to
  // zero or one to indicate that condition.
  long int is_main_namespace;

  // This count indicates how many users hold a reference
  // to this file:
  //   - the file has been dlopened (the dlopen increases the ref count)
  //   - the file is the main binary, loader or ld_preload binaries
  //     loaded during loader initialization
  // All other files have a count of zero.
  uint32_t count;
  char *name;
  dev_t st_dev;
  ino_t st_ino;
  struct VdlFileMap ro_map;
  struct VdlFileMap rw_map;
  // indicates if the deps field has been initialized correctly
  uint32_t deps_initialized : 1;
  // indicates if the has_tls field has been initialized correctly
  uint32_t tls_initialized : 1;
  // indicates if the ELF initializers of this file
  // have been called.
  uint32_t init_called : 1;
  // indicates if the ELF finalizers of this file
  // have been called.
  uint32_t fini_called : 1;
  // indicates if this file has been relocated
  uint32_t reloced : 1;
  // indicates if we patched this file for some
  // nastly glibc-isms.
  uint32_t patched : 1;
  // indicates if this represents the main executable.
  uint32_t is_executable : 1;
  uint32_t gc_color : 2;
  // indicates if this file has a TLS program entry
  // If so, all tls_-prefixed variables are valid.
  uint32_t has_tls : 1;
  // indicates whether this file is part of the static TLS
  // block
  uint32_t tls_is_static : 1;
  // start of TLS block template
  unsigned long tls_tmpl_start;
  // size of TLS block template
  unsigned long tls_tmpl_size;
  // the generation number when the tls template
  // of this number was initialized
  unsigned long tls_tmpl_gen;
  // size of TLS block zero area, located
  // right after the area initialized with the
  // TLS block template
  unsigned long tls_init_zero_size;
  // alignment requirements for the TLS block area
  unsigned long tls_align;
  // TLS module index associated to this file
  // this is the index in each thread's DTV
  // XXX: this member _must_ be at the same offset as l_tls_modid 
  // in the glibc linkmap to allow gdb to work (gdb accesses this
  // field for tls variable lookups)
  unsigned long tls_index;
  // offset from thread pointer to this module
  // this field is valid only for modules which
  // are loaded at startup.
  signed long tls_offset;
  // the list of objects in which we resolved a symbol 
  // from a GOT/PLT relocation. This field is used
  // during garbage collection from vdl_gc to detect
  // the set of references an object holds to another one
  // and thus avoid unloading an object which is held as a
  // reference by another object.
  struct VdlList *gc_symbols_resolved_in;
  enum VdlFileLookupType lookup_type;
  struct VdlContext *context;
  struct VdlList *local_scope;
  // list of files this file depends upon. 
  // equivalent to the content of DT_NEEDED.
  struct VdlList *deps;
  uint32_t depth;
};

ElfW(Dyn) *vdl_file_get_dynamic (const struct VdlFile *file, unsigned long tag);
unsigned long vdl_file_get_dynamic_v (const struct VdlFile *file, unsigned long tag);
unsigned long vdl_file_get_dynamic_p (const struct VdlFile *file, unsigned long tag);

#endif /* VDL_FILE_H */