#  Copyright (C) 2012  Statoil ASA, Norway.
#
#  The file 'field_config.py' is part of ERT - Ensemble based Reservoir Tool.
#
#  ERT is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  ERT is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE.
#
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
#  for more details.
import sys

from cwrap import BaseCClass
from res import ResPrototype
from ecl.util.util import StringList, IntegerHash


class CustomKWConfig(BaseCClass):
    TYPE_NAME = "custom_kw_config"

    _alloc_empty           = ResPrototype("void* custom_kw_config_alloc_empty(char*, char*, char*)", bind = False)
    _alloc_with_definition = ResPrototype("void* custom_kw_config_alloc_with_definition(char*, integer_hash)", bind = False)
    _get_name              = ResPrototype("char* custom_kw_config_get_name(custom_kw_config)")
    _get_result_file       = ResPrototype("char* custom_kw_config_get_result_file(custom_kw_config)")
    _get_output_file       = ResPrototype("char* custom_kw_config_get_output_file(custom_kw_config)")
    _parse_result_file     = ResPrototype("bool  custom_kw_config_parse_result_file(custom_kw_config, char*, stringlist)")
    _has_key               = ResPrototype("bool  custom_kw_config_has_key(custom_kw_config, char*)")
    _key_is_double         = ResPrototype("bool  custom_kw_config_key_is_double(custom_kw_config, char*)")
    _index_of_key          = ResPrototype("int   custom_kw_config_index_of_key(custom_kw_config, char*)")
    _size                  = ResPrototype("int   custom_kw_config_size(custom_kw_config)")
    _keys                  = ResPrototype("stringlist_obj custom_kw_config_get_alloc_keys(custom_kw_config)")
    _free                  = ResPrototype("void  custom_kw_config_free(custom_kw_config)")

    def __init__(self, key, result_file, output_file=None, definition=None):
        """
        @type key: str
        @type result_file: str
        @type output_file: str
        @type definition: dict
        """

        if definition is not None:
            if result_file is not None and output_file is not None:
                sys.stderr.write("[%s] Will ignore result file and output file when constructing with a definition." % self.__class__.__name__)

            type_hash = CustomKWConfig.convertDefinition(definition)
            c_ptr = self._alloc_with_definition(key, type_hash)
        else:
            c_ptr = self._alloc_empty(key, result_file, output_file)
        super(CustomKWConfig, self).__init__(c_ptr)

    def getName(self):
        """ @rtype: str """
        return self.name()

    def name(self):
        return self._get_name()

    def getResultFile(self):
        """ @rtype: str """
        return self._get_result_file()

    def getOutputFile(self):
        """ @rtype: str """
        return self._get_output_file()

    def parseResultFile(self, result_file, result):
        """ @rtype: bool """
        return self._parse_result_file(result_file, result)

    def keyIsDouble(self, key):
        """ @rtype: bool """
        return self._key_is_double(key)

    def indexOfKey(self, key):
        """ @rtype: int """
        return self._index_of_key(key)

    def __contains__(self, item):
        """ @rtype: bool """
        return self._has_key(item)

    def __len__(self):
        """ @rtype: int """
        return self._size()

    def __iter__(self):
        keys = self.getKeys()
        index = 0
        while index < len(keys):
            yield keys[index]
            index += 1

    def free(self):
       self._free()

    def __repr__(self):
        return 'CustomKWConfig(name = %s, len = %d) %s' % (self.name(), len(self), self._ad_str())

    def getKeys(self):
        """ @rtype: StringList """
        return self._keys()

    @classmethod
    def convertDefinition(cls, definition):
        """ @rtype: IntegerHash """
        type_hash = IntegerHash()

        for key, value_type in definition.items():
            if value_type == float:
                value_type = 1
            else:
                value_type = 0 #str
            type_hash[key] = value_type
        return type_hash
