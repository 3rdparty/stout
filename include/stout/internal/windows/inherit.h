// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <processthreadsapi.h>

#include "stout/error.h"
#include "stout/nothing.h"
#include "stout/os/int_fd.h"
#include "stout/try.h"
#include "stout/windows.h"

////////////////////////////////////////////////////////////////////////

namespace internal {

////////////////////////////////////////////////////////////////////////

namespace windows {

////////////////////////////////////////////////////////////////////////

// This function creates `LPPROC_THREAD_ATTRIBUTE_LIST`, which is used
// to whitelist handles sent to a child process.
typedef _PROC_THREAD_ATTRIBUTE_LIST AttributeList;

////////////////////////////////////////////////////////////////////////

inline Result<std::shared_ptr<AttributeList>>
create_attributes_list_for_handles(const std::vector<HANDLE>& handles) {
  if (handles.empty()) {
    return None();
  }

  SIZE_T size = 0;

  // BOOL InitializeProcThreadAttributeList(
  //    [out, optional] LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
  //    [in]            DWORD                        dwAttributeCount,
  //                    DWORD                        dwFlags,
  //    [in, out]       PSIZE_T                      lpSize
  // );
  //
  // [out, optional] lpAttributeList -
  //    Pointer to _PROC_THREAD_ATTRIBUTE_LIST. This
  //    parameter can be `nullptr` to determine the buffer
  //    size required to support the specified number of
  //    attributes.
  // [in] dwAttributeCount -
  //    Count of attributes to be added to the list.
  // dwFlags -
  //    This parameter is reserved and must be zero.
  // [in, out] lpSize -
  //    Size in bytes required for the attribute list.
  BOOL result = ::InitializeProcThreadAttributeList(nullptr, 1, 0, &size);

  if (result == FALSE) {
    if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
      return WindowsError();
    }
  }

  std::shared_ptr<AttributeList> attribute_list(
      reinterpret_cast<AttributeList*>(std::malloc(size)),
      [](AttributeList* p) {
        // NOTE: This delete API does not return anything, nor can it
        // fail, so it is safe to call it regardless of the
        // initialization state of `p`.
        ::DeleteProcThreadAttributeList(p);
        std::free(p);
      });

  if (attribute_list == nullptr) {
    // `std::malloc` failed.
    return WindowsError(ERROR_OUTOFMEMORY);
  }

  result =
      ::InitializeProcThreadAttributeList(attribute_list.get(), 1, 0, &size);

  if (result == FALSE) {
    return WindowsError();
  }

  // BOOL UpdateProcThreadAttribute(
  //  [in, out]       LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
  //  [in]            DWORD                        dwFlags,
  //  [in]            DWORD_PTR                    Attribute,
  //  [in]            PVOID                        lpValue,
  //  [in]            SIZE_T                       cbSize,
  //  [out, optional] PVOID                        lpPreviousValue,
  //  [in, optional]  PSIZE_T                      lpReturnSize
  // );
  //
  //  [in, out]  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList -
  //      A pointer to an attribute list created by the
  //      InitializeProcThreadAttributeList function.
  //  [in] dwFlags -
  //      This parameter is reserved and must be zero.
  //  [in] Attribute -
  //      The attribute key to update in the attribute list.
  //  [in] lpValue -
  //      A pointer to the attribute value. This value must persist
  //      until the attribute list is destroyed using the
  //      DeleteProcThreadAttributeList function.
  //  [in] cbSize -
  //      The size of the attribute value specified by the lpValue
  //      parameter.
  //  [out, optional] lpPreviousValue -
  //      This parameter is reserved and must be `nullptr`.
  //  [in, optional] lpReturnSize -
  //      This parameter is reserved and must be `nullptr`.
  result = ::UpdateProcThreadAttribute(
      attribute_list.get(),
      0,
      PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
      const_cast<PVOID*>(handles.data()),
      handles.size() * sizeof(HANDLE),
      nullptr,
      nullptr);

  if (result == FALSE) {
    return WindowsError();
  }

  return attribute_list;
}

////////////////////////////////////////////////////////////////////////

// This function enables or disables inheritance for a Windows file handle.
//
// NOTE: By default, handles on Windows are not inheritable, so this is
// primarily used to enable inheritance when passing handles to child
// processes, and subsequently disable inheritance.
inline Try<Nothing> set_inherit(const int_fd& fd, const bool inherit) {
  const BOOL result = ::SetHandleInformation(
      fd,
      HANDLE_FLAG_INHERIT,
      inherit ? HANDLE_FLAG_INHERIT : 0);

  if (result == FALSE) {
    return WindowsError();
  }

  return Nothing();
}

////////////////////////////////////////////////////////////////////////

} // namespace windows

////////////////////////////////////////////////////////////////////////

} // namespace internal

////////////////////////////////////////////////////////////////////////
