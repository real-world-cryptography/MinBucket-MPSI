# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.


target_sources(simple_hash
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/simple_hash.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../receiver/receiver_utils.cpp
)
