/* Copyright (C) 2015-2018 Codership Oy <info@codership.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */


#ifndef WSREP_SCHEMA_H
#define WSREP_SCHEMA_H

/* wsrep-lib */
#include "wsrep_types.h"


#include "mysqld.h"
#include "thr_lock.h" /* enum thr_lock_type */
#include "../wsrep/wsrep_api.h"
#include "wsrep_mysqld.h"

#include <string>

/*
  Forward decls
*/
class THD;
class Relay_log_info;
struct TABLE;
struct TABLE_LIST;
struct st_mysql_lex_string;
typedef struct st_mysql_lex_string LEX_STRING;

class Wsrep_thd_pool;

/** Name of the system database (schema) used for WSREP related data. */
extern const std::string wsrep_schema_str;

/** Name of the table in `wsrep_schema_str` used for storing streaming
replication data. In an InnoDB full format, e.g. "database/tablename". */
extern const std::string sr_table_name_full_str;

class Wsrep_schema
{
 public:

  Wsrep_schema(Wsrep_thd_pool*);
  ~Wsrep_schema();

  /*
    Initialize wsrep schema. Storage engines must be running before
    calling this function.
  */
  int init();

  /*
    Store wsrep view info into wsrep schema.
  */
  int store_view(THD*, const Wsrep_view& view);

  /*
    Restore view info from stable storage.
  */
  int restore_view(const wsrep_uuid_t& node_uuid, wsrep_view_info_t**) const;

  /*
    Append transaction fragment to fragment storage.
    Starts a trx using a THD from thd_pool, does not commit.
    Should be followed by a call to update_frag_seqno(), or
    release_SR_thd() if wsrep->certify() fails.
   */
  THD* append_frag(const wsrep_trx_meta_t&, uint32_t,
                   const unsigned char*, size_t);
  /**
    Append transaction fragment to fragment storage.
    Transaction must have been started for THD before this call.
    In order to make changes durable, transaction must be committed
    separately after this call.

    @param thd THD object
    @param server_id Wsrep server identifier
    @param transaction_id Transaction identifier
    @param flags Flags for the fragment
    @param data Fragment data buffer

    @return Zero in case of success, non-zero on failure.
  */
  int append_fragment(THD* thd,
                      const wsrep::id& server_id,
                      wsrep::transaction_id transaction_id,
                      wsrep::seqno seqno,
                      int flags,
                      const wsrep::const_buffer& data);
  /**
     Update existing fragment meta data. The fragment must have been
     inserted before using append_fragment().

     @param thd THD object
     @param ws_meta Wsrep meta data

     @return Zero in case of success, non-zero on failure.
   */
  int update_fragment_meta(THD* thd,
                           const wsrep::ws_meta& ws_meta);

  /**
     Remove fragments from storage. This method must be called
     inside active transaction. Fragment removal will be committed
     once the transaction commits.

     @param thd Pointer to THD object
     @param server_id Identifier of the running server
     @param transaction_id Identifier of the current transaction
     @param fragments Vector of fragment seqnos to be removed
  */
  int remove_fragments(THD*                             thd,
                       const wsrep::id&                 server_id,
                       wsrep::transaction_id            transaction_id,
                       const std::vector<wsrep::seqno>& fragments);

  /**
     Replay a transaction from stored fragments. The caller must have
     started a transaction for a thd.

     @param thd Pointer to THD object
     @param ws_meta Write set meta data for commit fragment.
     @param fragments Vector of fragments to be replayed

     @return Zero on success, non-zero on failure.
  */
  int replay_transaction(THD* thd,
                         Relay_log_info* rli,
                         const wsrep::ws_meta& ws_meta,
                         const std::vector<wsrep::seqno>& fragments);

  /**
     Recover streaming transactions from SR table.
     This method should be called after storage enignes are initialized.
     It will scan SR table and replay found streaming transactions.

     @return Zero on success, non-zero on failure.
  */
  int recover_sr_transactions();

  /*
    Close wsrep schema.
  */
  void close();

 private:
  /* Non-copyable */
  Wsrep_schema(const Wsrep_schema&);
  Wsrep_schema& operator=(const Wsrep_schema&);

  Wsrep_thd_pool* thd_pool_;
};

extern Wsrep_schema* wsrep_schema;

#endif /* !WSREP_SCHEMA_H */
