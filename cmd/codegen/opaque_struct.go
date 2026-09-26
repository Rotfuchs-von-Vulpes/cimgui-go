package main

import (
	"encoding/json"
	"fmt"
)

type OpaqueStructsSection struct {
	OpaqueStructs json.RawMessage `json:"opaque_structs"`
}

func getOpaqueStructs(jsonBytes []byte) (result map[CIdentifier]string, err error) {
	var osSection OpaqueStructsSection
	err = json.Unmarshal(jsonBytes, &osSection)
	if err != nil {
		return nil, fmt.Errorf("cannot extract opaque structs section from json: %w", err)
	}

	if len(osSection.OpaqueStructs) <= 2 { // empty or empty array
		return make(map[CIdentifier]string), nil
	}

	if err := json.Unmarshal(osSection.OpaqueStructs, &result); err != nil {
		return nil, fmt.Errorf("cannot extract opaque structs from json section: %w", err)
	}

	for k := range result {
		result[k] = fmt.Sprintf("struct %s", k)
	}

	return result, nil
}
