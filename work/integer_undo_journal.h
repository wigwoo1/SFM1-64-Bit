#pragma once
#include <variant>
#include <memory>
#include "scalar_reference_undo_record.h"
#include <type_traits>
#include "runtime_undo_journal.h"
#include "integer_clear_undo_record.h"
#include "integer_append_undo_record.h"
using RuntimeIntegerUndoRecord=std::variant<RuntimeIntegerClearRecord,RuntimeIntegerAppendRecord,std::shared_ptr<RuntimeScalarReferenceUndoRecord>>;
using RuntimeIntegerUndoJournal=RuntimeUndoJournal<RuntimeIntegerUndoRecord>;
template<class Notify,class Suppressed,class Write>
size_t replayMixedJournal(RuntimeIntegerUndoJournal&journal,SessionRuntime&r,MutationCoordinator&m,bool reverse,uint32_t&dirty,bool preparationSuppressed,Notify notify,Suppressed suppressed,Write write){
 auto replay=[&](const RuntimeIntegerUndoRecord&entry,bool backwards){std::visit([&](const auto&record){
  using T=std::decay_t<decltype(record)>;
  if constexpr(std::is_same_v<T,RuntimeIntegerClearRecord>)replayIntegerClearRecord(r,m,record,backwards,dirty,preparationSuppressed,notify,suppressed);
  else if constexpr(std::is_same_v<T,RuntimeIntegerAppendRecord>)replayIntegerAppendRecord(r,m,record,backwards,dirty,preparationSuppressed,notify,suppressed);
  else {if(!record)throw std::runtime_error("missing scalar undo record");replayScalarReferenceRecord(r,m,*record,backwards,write);}
 },entry);};
 return reverse?journal.reverse(replay):journal.forward(replay);
}
template<class Notify,class Suppressed>
size_t replayIntegerJournal(RuntimeIntegerUndoJournal&journal,SessionRuntime&r,MutationCoordinator&m,bool reverse,uint32_t&dirty,bool preparationSuppressed,Notify notify,Suppressed suppressed){
 auto replay=[&](const RuntimeIntegerUndoRecord&entry,bool backwards){std::visit([&](const auto&record){
  using T=std::decay_t<decltype(record)>;
  if constexpr(std::is_same_v<T,RuntimeIntegerClearRecord>)replayIntegerClearRecord(r,m,record,backwards,dirty,preparationSuppressed,notify,suppressed);
  else if constexpr(std::is_same_v<T,RuntimeIntegerAppendRecord>)replayIntegerAppendRecord(r,m,record,backwards,dirty,preparationSuppressed,notify,suppressed);
  else throw std::runtime_error("scalar record requires class-specific journal replay");
 },entry);};
 return reverse?journal.reverse(replay):journal.forward(replay);
}

