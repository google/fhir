//    Copyright 2023 Google Inc.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        https://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

// Code generated by protoc-gen-go. DO NOT EDIT.
// versions:
// 	protoc-gen-go v1.29.0
// 	protoc        v3.21.12
// source: proto/google/fhir/proto/r5/core/resources/encounter_history.proto

package encounter_history_go_proto

import (
	_ "github.com/google/fhir/go/proto/google/fhir/proto/annotations_go_proto"
	codes_go_proto "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/codes_go_proto"
	datatypes_go_proto "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/datatypes_go_proto"
	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	protoimpl "google.golang.org/protobuf/runtime/protoimpl"
	anypb "google.golang.org/protobuf/types/known/anypb"
	reflect "reflect"
	sync "sync"
)

const (
	// Verify that this generated code is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(20 - protoimpl.MinVersion)
	// Verify that runtime/protoimpl is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(protoimpl.MaxVersion - 20)
)

// Auto-generated from StructureDefinition for EncounterHistory.
// A record of significant events/milestones key data throughout the history of
// an Encounter. See http://hl7.org/fhir/StructureDefinition/EncounterHistory
type EncounterHistory struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// Logical id of this artifact
	Id *datatypes_go_proto.Id `protobuf:"bytes,1,opt,name=id,proto3" json:"id,omitempty"`
	// Metadata about the resource
	Meta *datatypes_go_proto.Meta `protobuf:"bytes,2,opt,name=meta,proto3" json:"meta,omitempty"`
	// A set of rules under which this content was created
	ImplicitRules *datatypes_go_proto.Uri `protobuf:"bytes,3,opt,name=implicit_rules,json=implicitRules,proto3" json:"implicit_rules,omitempty"`
	// Language of the resource content
	Language *datatypes_go_proto.Code `protobuf:"bytes,4,opt,name=language,proto3" json:"language,omitempty"`
	// Text summary of the resource, for human interpretation
	Text *datatypes_go_proto.Narrative `protobuf:"bytes,5,opt,name=text,proto3" json:"text,omitempty"`
	// Contained, inline Resources
	Contained []*anypb.Any `protobuf:"bytes,6,rep,name=contained,proto3" json:"contained,omitempty"`
	// Additional content defined by implementations
	Extension []*datatypes_go_proto.Extension `protobuf:"bytes,8,rep,name=extension,proto3" json:"extension,omitempty"`
	// Extensions that cannot be ignored
	ModifierExtension []*datatypes_go_proto.Extension `protobuf:"bytes,9,rep,name=modifier_extension,json=modifierExtension,proto3" json:"modifier_extension,omitempty"`
	// The Encounter associated with this set of historic values
	Encounter *datatypes_go_proto.Reference `protobuf:"bytes,10,opt,name=encounter,proto3" json:"encounter,omitempty"`
	// Identifier(s) by which this encounter is known
	Identifier []*datatypes_go_proto.Identifier `protobuf:"bytes,11,rep,name=identifier,proto3" json:"identifier,omitempty"`
	Status     *EncounterHistory_StatusCode     `protobuf:"bytes,12,opt,name=status,proto3" json:"status,omitempty"`
	// Classification of patient encounter
	ClassValue *datatypes_go_proto.CodeableConcept `protobuf:"bytes,13,opt,name=class_value,json=class,proto3" json:"class_value,omitempty"`
	// Specific type of encounter
	Type []*datatypes_go_proto.CodeableConcept `protobuf:"bytes,14,rep,name=type,proto3" json:"type,omitempty"`
	// Specific type of service
	ServiceType []*datatypes_go_proto.CodeableReference `protobuf:"bytes,15,rep,name=service_type,json=serviceType,proto3" json:"service_type,omitempty"`
	// The patient or group related to this encounter
	Subject *datatypes_go_proto.Reference `protobuf:"bytes,16,opt,name=subject,proto3" json:"subject,omitempty"`
	// The current status of the subject in relation to the Encounter
	SubjectStatus *datatypes_go_proto.CodeableConcept `protobuf:"bytes,17,opt,name=subject_status,json=subjectStatus,proto3" json:"subject_status,omitempty"`
	// The actual start and end time associated with this set of values associated
	// with the encounter
	ActualPeriod *datatypes_go_proto.Period `protobuf:"bytes,18,opt,name=actual_period,json=actualPeriod,proto3" json:"actual_period,omitempty"`
	// The planned start date/time (or admission date) of the encounter
	PlannedStartDate *datatypes_go_proto.DateTime `protobuf:"bytes,19,opt,name=planned_start_date,json=plannedStartDate,proto3" json:"planned_start_date,omitempty"`
	// The planned end date/time (or discharge date) of the encounter
	PlannedEndDate *datatypes_go_proto.DateTime `protobuf:"bytes,20,opt,name=planned_end_date,json=plannedEndDate,proto3" json:"planned_end_date,omitempty"`
	// Actual quantity of time the encounter lasted (less time absent)
	Length   *datatypes_go_proto.Duration `protobuf:"bytes,21,opt,name=length,proto3" json:"length,omitempty"`
	Location []*EncounterHistory_Location `protobuf:"bytes,22,rep,name=location,proto3" json:"location,omitempty"`
}

func (x *EncounterHistory) Reset() {
	*x = EncounterHistory{}
	if protoimpl.UnsafeEnabled {
		mi := &file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *EncounterHistory) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*EncounterHistory) ProtoMessage() {}

func (x *EncounterHistory) ProtoReflect() protoreflect.Message {
	mi := &file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use EncounterHistory.ProtoReflect.Descriptor instead.
func (*EncounterHistory) Descriptor() ([]byte, []int) {
	return file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDescGZIP(), []int{0}
}

func (x *EncounterHistory) GetId() *datatypes_go_proto.Id {
	if x != nil {
		return x.Id
	}
	return nil
}

func (x *EncounterHistory) GetMeta() *datatypes_go_proto.Meta {
	if x != nil {
		return x.Meta
	}
	return nil
}

func (x *EncounterHistory) GetImplicitRules() *datatypes_go_proto.Uri {
	if x != nil {
		return x.ImplicitRules
	}
	return nil
}

func (x *EncounterHistory) GetLanguage() *datatypes_go_proto.Code {
	if x != nil {
		return x.Language
	}
	return nil
}

func (x *EncounterHistory) GetText() *datatypes_go_proto.Narrative {
	if x != nil {
		return x.Text
	}
	return nil
}

func (x *EncounterHistory) GetContained() []*anypb.Any {
	if x != nil {
		return x.Contained
	}
	return nil
}

func (x *EncounterHistory) GetExtension() []*datatypes_go_proto.Extension {
	if x != nil {
		return x.Extension
	}
	return nil
}

func (x *EncounterHistory) GetModifierExtension() []*datatypes_go_proto.Extension {
	if x != nil {
		return x.ModifierExtension
	}
	return nil
}

func (x *EncounterHistory) GetEncounter() *datatypes_go_proto.Reference {
	if x != nil {
		return x.Encounter
	}
	return nil
}

func (x *EncounterHistory) GetIdentifier() []*datatypes_go_proto.Identifier {
	if x != nil {
		return x.Identifier
	}
	return nil
}

func (x *EncounterHistory) GetStatus() *EncounterHistory_StatusCode {
	if x != nil {
		return x.Status
	}
	return nil
}

func (x *EncounterHistory) GetClassValue() *datatypes_go_proto.CodeableConcept {
	if x != nil {
		return x.ClassValue
	}
	return nil
}

func (x *EncounterHistory) GetType() []*datatypes_go_proto.CodeableConcept {
	if x != nil {
		return x.Type
	}
	return nil
}

func (x *EncounterHistory) GetServiceType() []*datatypes_go_proto.CodeableReference {
	if x != nil {
		return x.ServiceType
	}
	return nil
}

func (x *EncounterHistory) GetSubject() *datatypes_go_proto.Reference {
	if x != nil {
		return x.Subject
	}
	return nil
}

func (x *EncounterHistory) GetSubjectStatus() *datatypes_go_proto.CodeableConcept {
	if x != nil {
		return x.SubjectStatus
	}
	return nil
}

func (x *EncounterHistory) GetActualPeriod() *datatypes_go_proto.Period {
	if x != nil {
		return x.ActualPeriod
	}
	return nil
}

func (x *EncounterHistory) GetPlannedStartDate() *datatypes_go_proto.DateTime {
	if x != nil {
		return x.PlannedStartDate
	}
	return nil
}

func (x *EncounterHistory) GetPlannedEndDate() *datatypes_go_proto.DateTime {
	if x != nil {
		return x.PlannedEndDate
	}
	return nil
}

func (x *EncounterHistory) GetLength() *datatypes_go_proto.Duration {
	if x != nil {
		return x.Length
	}
	return nil
}

func (x *EncounterHistory) GetLocation() []*EncounterHistory_Location {
	if x != nil {
		return x.Location
	}
	return nil
}

// planned | in-progress | on-hold | discharged | completed | cancelled |
// discontinued | entered-in-error | unknown
type EncounterHistory_StatusCode struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	Value     codes_go_proto.EncounterStatusCode_Value `protobuf:"varint,1,opt,name=value,proto3,enum=google.fhir.r5.core.EncounterStatusCode_Value" json:"value,omitempty"`
	Id        *datatypes_go_proto.String               `protobuf:"bytes,2,opt,name=id,proto3" json:"id,omitempty"`
	Extension []*datatypes_go_proto.Extension          `protobuf:"bytes,3,rep,name=extension,proto3" json:"extension,omitempty"`
}

func (x *EncounterHistory_StatusCode) Reset() {
	*x = EncounterHistory_StatusCode{}
	if protoimpl.UnsafeEnabled {
		mi := &file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_msgTypes[1]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *EncounterHistory_StatusCode) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*EncounterHistory_StatusCode) ProtoMessage() {}

func (x *EncounterHistory_StatusCode) ProtoReflect() protoreflect.Message {
	mi := &file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_msgTypes[1]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use EncounterHistory_StatusCode.ProtoReflect.Descriptor instead.
func (*EncounterHistory_StatusCode) Descriptor() ([]byte, []int) {
	return file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDescGZIP(), []int{0, 0}
}

func (x *EncounterHistory_StatusCode) GetValue() codes_go_proto.EncounterStatusCode_Value {
	if x != nil {
		return x.Value
	}
	return codes_go_proto.EncounterStatusCode_Value(0)
}

func (x *EncounterHistory_StatusCode) GetId() *datatypes_go_proto.String {
	if x != nil {
		return x.Id
	}
	return nil
}

func (x *EncounterHistory_StatusCode) GetExtension() []*datatypes_go_proto.Extension {
	if x != nil {
		return x.Extension
	}
	return nil
}

// Location of the patient at this point in the encounter
type EncounterHistory_Location struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// Unique id for inter-element referencing
	Id *datatypes_go_proto.String `protobuf:"bytes,1,opt,name=id,proto3" json:"id,omitempty"`
	// Additional content defined by implementations
	Extension []*datatypes_go_proto.Extension `protobuf:"bytes,2,rep,name=extension,proto3" json:"extension,omitempty"`
	// Extensions that cannot be ignored even if unrecognized
	ModifierExtension []*datatypes_go_proto.Extension `protobuf:"bytes,3,rep,name=modifier_extension,json=modifierExtension,proto3" json:"modifier_extension,omitempty"`
	// Location the encounter takes place
	Location *datatypes_go_proto.Reference `protobuf:"bytes,4,opt,name=location,proto3" json:"location,omitempty"`
	// The physical type of the location (usually the level in the location
	// hierarchy - bed, room, ward, virtual etc.)
	Form *datatypes_go_proto.CodeableConcept `protobuf:"bytes,5,opt,name=form,proto3" json:"form,omitempty"`
}

func (x *EncounterHistory_Location) Reset() {
	*x = EncounterHistory_Location{}
	if protoimpl.UnsafeEnabled {
		mi := &file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_msgTypes[2]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *EncounterHistory_Location) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*EncounterHistory_Location) ProtoMessage() {}

func (x *EncounterHistory_Location) ProtoReflect() protoreflect.Message {
	mi := &file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_msgTypes[2]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use EncounterHistory_Location.ProtoReflect.Descriptor instead.
func (*EncounterHistory_Location) Descriptor() ([]byte, []int) {
	return file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDescGZIP(), []int{0, 1}
}

func (x *EncounterHistory_Location) GetId() *datatypes_go_proto.String {
	if x != nil {
		return x.Id
	}
	return nil
}

func (x *EncounterHistory_Location) GetExtension() []*datatypes_go_proto.Extension {
	if x != nil {
		return x.Extension
	}
	return nil
}

func (x *EncounterHistory_Location) GetModifierExtension() []*datatypes_go_proto.Extension {
	if x != nil {
		return x.ModifierExtension
	}
	return nil
}

func (x *EncounterHistory_Location) GetLocation() *datatypes_go_proto.Reference {
	if x != nil {
		return x.Location
	}
	return nil
}

func (x *EncounterHistory_Location) GetForm() *datatypes_go_proto.CodeableConcept {
	if x != nil {
		return x.Form
	}
	return nil
}

var File_proto_google_fhir_proto_r5_core_resources_encounter_history_proto protoreflect.FileDescriptor

var file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDesc = []byte{
	0x0a, 0x41, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2f, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2f, 0x66,
	0x68, 0x69, 0x72, 0x2f, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2f, 0x72, 0x35, 0x2f, 0x63, 0x6f, 0x72,
	0x65, 0x2f, 0x72, 0x65, 0x73, 0x6f, 0x75, 0x72, 0x63, 0x65, 0x73, 0x2f, 0x65, 0x6e, 0x63, 0x6f,
	0x75, 0x6e, 0x74, 0x65, 0x72, 0x5f, 0x68, 0x69, 0x73, 0x74, 0x6f, 0x72, 0x79, 0x2e, 0x70, 0x72,
	0x6f, 0x74, 0x6f, 0x12, 0x13, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72,
	0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x1a, 0x19, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
	0x2f, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x62, 0x75, 0x66, 0x2f, 0x61, 0x6e, 0x79, 0x2e, 0x70, 0x72,
	0x6f, 0x74, 0x6f, 0x1a, 0x29, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2f, 0x67, 0x6f, 0x6f, 0x67, 0x6c,
	0x65, 0x2f, 0x66, 0x68, 0x69, 0x72, 0x2f, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2f, 0x61, 0x6e, 0x6e,
	0x6f, 0x74, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x1a, 0x2b,
	0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2f, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2f, 0x66, 0x68, 0x69,
	0x72, 0x2f, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2f, 0x72, 0x35, 0x2f, 0x63, 0x6f, 0x72, 0x65, 0x2f,
	0x63, 0x6f, 0x64, 0x65, 0x73, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x1a, 0x2f, 0x70, 0x72, 0x6f,
	0x74, 0x6f, 0x2f, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2f, 0x66, 0x68, 0x69, 0x72, 0x2f, 0x70,
	0x72, 0x6f, 0x74, 0x6f, 0x2f, 0x72, 0x35, 0x2f, 0x63, 0x6f, 0x72, 0x65, 0x2f, 0x64, 0x61, 0x74,
	0x61, 0x74, 0x79, 0x70, 0x65, 0x73, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x22, 0xd0, 0x10, 0x0a,
	0x10, 0x45, 0x6e, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72, 0x48, 0x69, 0x73, 0x74, 0x6f, 0x72,
	0x79, 0x12, 0x27, 0x0a, 0x02, 0x69, 0x64, 0x18, 0x01, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x17, 0x2e,
	0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63,
	0x6f, 0x72, 0x65, 0x2e, 0x49, 0x64, 0x52, 0x02, 0x69, 0x64, 0x12, 0x2d, 0x0a, 0x04, 0x6d, 0x65,
	0x74, 0x61, 0x18, 0x02, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x19, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c,
	0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x4d,
	0x65, 0x74, 0x61, 0x52, 0x04, 0x6d, 0x65, 0x74, 0x61, 0x12, 0x3f, 0x0a, 0x0e, 0x69, 0x6d, 0x70,
	0x6c, 0x69, 0x63, 0x69, 0x74, 0x5f, 0x72, 0x75, 0x6c, 0x65, 0x73, 0x18, 0x03, 0x20, 0x01, 0x28,
	0x0b, 0x32, 0x18, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e,
	0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x55, 0x72, 0x69, 0x52, 0x0d, 0x69, 0x6d, 0x70,
	0x6c, 0x69, 0x63, 0x69, 0x74, 0x52, 0x75, 0x6c, 0x65, 0x73, 0x12, 0x35, 0x0a, 0x08, 0x6c, 0x61,
	0x6e, 0x67, 0x75, 0x61, 0x67, 0x65, 0x18, 0x04, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x19, 0x2e, 0x67,
	0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f,
	0x72, 0x65, 0x2e, 0x43, 0x6f, 0x64, 0x65, 0x52, 0x08, 0x6c, 0x61, 0x6e, 0x67, 0x75, 0x61, 0x67,
	0x65, 0x12, 0x32, 0x0a, 0x04, 0x74, 0x65, 0x78, 0x74, 0x18, 0x05, 0x20, 0x01, 0x28, 0x0b, 0x32,
	0x1e, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35,
	0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x4e, 0x61, 0x72, 0x72, 0x61, 0x74, 0x69, 0x76, 0x65, 0x52,
	0x04, 0x74, 0x65, 0x78, 0x74, 0x12, 0x32, 0x0a, 0x09, 0x63, 0x6f, 0x6e, 0x74, 0x61, 0x69, 0x6e,
	0x65, 0x64, 0x18, 0x06, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x14, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c,
	0x65, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x62, 0x75, 0x66, 0x2e, 0x41, 0x6e, 0x79, 0x52, 0x09,
	0x63, 0x6f, 0x6e, 0x74, 0x61, 0x69, 0x6e, 0x65, 0x64, 0x12, 0x3c, 0x0a, 0x09, 0x65, 0x78, 0x74,
	0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x18, 0x08, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x1e, 0x2e, 0x67,
	0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f,
	0x72, 0x65, 0x2e, 0x45, 0x78, 0x74, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x52, 0x09, 0x65, 0x78,
	0x74, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x12, 0x4d, 0x0a, 0x12, 0x6d, 0x6f, 0x64, 0x69, 0x66,
	0x69, 0x65, 0x72, 0x5f, 0x65, 0x78, 0x74, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x18, 0x09, 0x20,
	0x03, 0x28, 0x0b, 0x32, 0x1e, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69,
	0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x45, 0x78, 0x74, 0x65, 0x6e, 0x73,
	0x69, 0x6f, 0x6e, 0x52, 0x11, 0x6d, 0x6f, 0x64, 0x69, 0x66, 0x69, 0x65, 0x72, 0x45, 0x78, 0x74,
	0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x12, 0x4d, 0x0a, 0x09, 0x65, 0x6e, 0x63, 0x6f, 0x75, 0x6e,
	0x74, 0x65, 0x72, 0x18, 0x0a, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x1e, 0x2e, 0x67, 0x6f, 0x6f, 0x67,
	0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e,
	0x52, 0x65, 0x66, 0x65, 0x72, 0x65, 0x6e, 0x63, 0x65, 0x42, 0x0f, 0xf2, 0xff, 0xfc, 0xc2, 0x06,
	0x09, 0x45, 0x6e, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72, 0x52, 0x09, 0x65, 0x6e, 0x63, 0x6f,
	0x75, 0x6e, 0x74, 0x65, 0x72, 0x12, 0x3f, 0x0a, 0x0a, 0x69, 0x64, 0x65, 0x6e, 0x74, 0x69, 0x66,
	0x69, 0x65, 0x72, 0x18, 0x0b, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x1f, 0x2e, 0x67, 0x6f, 0x6f, 0x67,
	0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e,
	0x49, 0x64, 0x65, 0x6e, 0x74, 0x69, 0x66, 0x69, 0x65, 0x72, 0x52, 0x0a, 0x69, 0x64, 0x65, 0x6e,
	0x74, 0x69, 0x66, 0x69, 0x65, 0x72, 0x12, 0x50, 0x0a, 0x06, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73,
	0x18, 0x0c, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x30, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e,
	0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x45, 0x6e, 0x63,
	0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72, 0x48, 0x69, 0x73, 0x74, 0x6f, 0x72, 0x79, 0x2e, 0x53, 0x74,
	0x61, 0x74, 0x75, 0x73, 0x43, 0x6f, 0x64, 0x65, 0x42, 0x06, 0xf0, 0xd0, 0x87, 0xeb, 0x04, 0x01,
	0x52, 0x06, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73, 0x12, 0x48, 0x0a, 0x0b, 0x63, 0x6c, 0x61, 0x73,
	0x73, 0x5f, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x18, 0x0d, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x24, 0x2e,
	0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63,
	0x6f, 0x72, 0x65, 0x2e, 0x43, 0x6f, 0x64, 0x65, 0x61, 0x62, 0x6c, 0x65, 0x43, 0x6f, 0x6e, 0x63,
	0x65, 0x70, 0x74, 0x42, 0x06, 0xf0, 0xd0, 0x87, 0xeb, 0x04, 0x01, 0x52, 0x05, 0x63, 0x6c, 0x61,
	0x73, 0x73, 0x12, 0x38, 0x0a, 0x04, 0x74, 0x79, 0x70, 0x65, 0x18, 0x0e, 0x20, 0x03, 0x28, 0x0b,
	0x32, 0x24, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72,
	0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x43, 0x6f, 0x64, 0x65, 0x61, 0x62, 0x6c, 0x65, 0x43,
	0x6f, 0x6e, 0x63, 0x65, 0x70, 0x74, 0x52, 0x04, 0x74, 0x79, 0x70, 0x65, 0x12, 0x49, 0x0a, 0x0c,
	0x73, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x5f, 0x74, 0x79, 0x70, 0x65, 0x18, 0x0f, 0x20, 0x03,
	0x28, 0x0b, 0x32, 0x26, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72,
	0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x43, 0x6f, 0x64, 0x65, 0x61, 0x62, 0x6c,
	0x65, 0x52, 0x65, 0x66, 0x65, 0x72, 0x65, 0x6e, 0x63, 0x65, 0x52, 0x0b, 0x73, 0x65, 0x72, 0x76,
	0x69, 0x63, 0x65, 0x54, 0x79, 0x70, 0x65, 0x12, 0x52, 0x0a, 0x07, 0x73, 0x75, 0x62, 0x6a, 0x65,
	0x63, 0x74, 0x18, 0x10, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x1e, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c,
	0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x52,
	0x65, 0x66, 0x65, 0x72, 0x65, 0x6e, 0x63, 0x65, 0x42, 0x18, 0xf2, 0xff, 0xfc, 0xc2, 0x06, 0x07,
	0x50, 0x61, 0x74, 0x69, 0x65, 0x6e, 0x74, 0xf2, 0xff, 0xfc, 0xc2, 0x06, 0x05, 0x47, 0x72, 0x6f,
	0x75, 0x70, 0x52, 0x07, 0x73, 0x75, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x12, 0x4b, 0x0a, 0x0e, 0x73,
	0x75, 0x62, 0x6a, 0x65, 0x63, 0x74, 0x5f, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73, 0x18, 0x11, 0x20,
	0x01, 0x28, 0x0b, 0x32, 0x24, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69,
	0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x43, 0x6f, 0x64, 0x65, 0x61, 0x62,
	0x6c, 0x65, 0x43, 0x6f, 0x6e, 0x63, 0x65, 0x70, 0x74, 0x52, 0x0d, 0x73, 0x75, 0x62, 0x6a, 0x65,
	0x63, 0x74, 0x53, 0x74, 0x61, 0x74, 0x75, 0x73, 0x12, 0x40, 0x0a, 0x0d, 0x61, 0x63, 0x74, 0x75,
	0x61, 0x6c, 0x5f, 0x70, 0x65, 0x72, 0x69, 0x6f, 0x64, 0x18, 0x12, 0x20, 0x01, 0x28, 0x0b, 0x32,
	0x1b, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35,
	0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x50, 0x65, 0x72, 0x69, 0x6f, 0x64, 0x52, 0x0c, 0x61, 0x63,
	0x74, 0x75, 0x61, 0x6c, 0x50, 0x65, 0x72, 0x69, 0x6f, 0x64, 0x12, 0x4b, 0x0a, 0x12, 0x70, 0x6c,
	0x61, 0x6e, 0x6e, 0x65, 0x64, 0x5f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x5f, 0x64, 0x61, 0x74, 0x65,
	0x18, 0x13, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x1d, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e,
	0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x44, 0x61, 0x74,
	0x65, 0x54, 0x69, 0x6d, 0x65, 0x52, 0x10, 0x70, 0x6c, 0x61, 0x6e, 0x6e, 0x65, 0x64, 0x53, 0x74,
	0x61, 0x72, 0x74, 0x44, 0x61, 0x74, 0x65, 0x12, 0x47, 0x0a, 0x10, 0x70, 0x6c, 0x61, 0x6e, 0x6e,
	0x65, 0x64, 0x5f, 0x65, 0x6e, 0x64, 0x5f, 0x64, 0x61, 0x74, 0x65, 0x18, 0x14, 0x20, 0x01, 0x28,
	0x0b, 0x32, 0x1d, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e,
	0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x44, 0x61, 0x74, 0x65, 0x54, 0x69, 0x6d, 0x65,
	0x52, 0x0e, 0x70, 0x6c, 0x61, 0x6e, 0x6e, 0x65, 0x64, 0x45, 0x6e, 0x64, 0x44, 0x61, 0x74, 0x65,
	0x12, 0x35, 0x0a, 0x06, 0x6c, 0x65, 0x6e, 0x67, 0x74, 0x68, 0x18, 0x15, 0x20, 0x01, 0x28, 0x0b,
	0x32, 0x1d, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72,
	0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x44, 0x75, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x52,
	0x06, 0x6c, 0x65, 0x6e, 0x67, 0x74, 0x68, 0x12, 0x4a, 0x0a, 0x08, 0x6c, 0x6f, 0x63, 0x61, 0x74,
	0x69, 0x6f, 0x6e, 0x18, 0x16, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x2e, 0x2e, 0x67, 0x6f, 0x6f, 0x67,
	0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e,
	0x45, 0x6e, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72, 0x48, 0x69, 0x73, 0x74, 0x6f, 0x72, 0x79,
	0x2e, 0x4c, 0x6f, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x52, 0x08, 0x6c, 0x6f, 0x63, 0x61, 0x74,
	0x69, 0x6f, 0x6e, 0x1a, 0xaa, 0x02, 0x0a, 0x0a, 0x53, 0x74, 0x61, 0x74, 0x75, 0x73, 0x43, 0x6f,
	0x64, 0x65, 0x12, 0x44, 0x0a, 0x05, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x18, 0x01, 0x20, 0x01, 0x28,
	0x0e, 0x32, 0x2e, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e,
	0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x45, 0x6e, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x65,
	0x72, 0x53, 0x74, 0x61, 0x74, 0x75, 0x73, 0x43, 0x6f, 0x64, 0x65, 0x2e, 0x56, 0x61, 0x6c, 0x75,
	0x65, 0x52, 0x05, 0x76, 0x61, 0x6c, 0x75, 0x65, 0x12, 0x2b, 0x0a, 0x02, 0x69, 0x64, 0x18, 0x02,
	0x20, 0x01, 0x28, 0x0b, 0x32, 0x1b, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68,
	0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x53, 0x74, 0x72, 0x69, 0x6e,
	0x67, 0x52, 0x02, 0x69, 0x64, 0x12, 0x3c, 0x0a, 0x09, 0x65, 0x78, 0x74, 0x65, 0x6e, 0x73, 0x69,
	0x6f, 0x6e, 0x18, 0x03, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x1e, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c,
	0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x45,
	0x78, 0x74, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x52, 0x09, 0x65, 0x78, 0x74, 0x65, 0x6e, 0x73,
	0x69, 0x6f, 0x6e, 0x3a, 0x6b, 0x8a, 0xf9, 0x83, 0xb2, 0x05, 0x2d, 0x68, 0x74, 0x74, 0x70, 0x3a,
	0x2f, 0x2f, 0x68, 0x6c, 0x37, 0x2e, 0x6f, 0x72, 0x67, 0x2f, 0x66, 0x68, 0x69, 0x72, 0x2f, 0x56,
	0x61, 0x6c, 0x75, 0x65, 0x53, 0x65, 0x74, 0x2f, 0x65, 0x6e, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x65,
	0x72, 0x2d, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73, 0xc0, 0x9f, 0xe3, 0xb6, 0x05, 0x01, 0x9a, 0xb5,
	0x8e, 0x93, 0x06, 0x2c, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x68, 0x6c, 0x37, 0x2e, 0x6f,
	0x72, 0x67, 0x2f, 0x66, 0x68, 0x69, 0x72, 0x2f, 0x53, 0x74, 0x72, 0x75, 0x63, 0x74, 0x75, 0x72,
	0x65, 0x44, 0x65, 0x66, 0x69, 0x6e, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x2f, 0x63, 0x6f, 0x64, 0x65,
	0x1a, 0xd0, 0x02, 0x0a, 0x08, 0x4c, 0x6f, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x12, 0x2b, 0x0a,
	0x02, 0x69, 0x64, 0x18, 0x01, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x1b, 0x2e, 0x67, 0x6f, 0x6f, 0x67,
	0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e,
	0x53, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x52, 0x02, 0x69, 0x64, 0x12, 0x3c, 0x0a, 0x09, 0x65, 0x78,
	0x74, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x18, 0x02, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x1e, 0x2e,
	0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63,
	0x6f, 0x72, 0x65, 0x2e, 0x45, 0x78, 0x74, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x52, 0x09, 0x65,
	0x78, 0x74, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x12, 0x4d, 0x0a, 0x12, 0x6d, 0x6f, 0x64, 0x69,
	0x66, 0x69, 0x65, 0x72, 0x5f, 0x65, 0x78, 0x74, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x18, 0x03,
	0x20, 0x03, 0x28, 0x0b, 0x32, 0x1e, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68,
	0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x45, 0x78, 0x74, 0x65, 0x6e,
	0x73, 0x69, 0x6f, 0x6e, 0x52, 0x11, 0x6d, 0x6f, 0x64, 0x69, 0x66, 0x69, 0x65, 0x72, 0x45, 0x78,
	0x74, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x12, 0x50, 0x0a, 0x08, 0x6c, 0x6f, 0x63, 0x61, 0x74,
	0x69, 0x6f, 0x6e, 0x18, 0x04, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x1e, 0x2e, 0x67, 0x6f, 0x6f, 0x67,
	0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e,
	0x52, 0x65, 0x66, 0x65, 0x72, 0x65, 0x6e, 0x63, 0x65, 0x42, 0x14, 0xf0, 0xd0, 0x87, 0xeb, 0x04,
	0x01, 0xf2, 0xff, 0xfc, 0xc2, 0x06, 0x08, 0x4c, 0x6f, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x52,
	0x08, 0x6c, 0x6f, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x12, 0x38, 0x0a, 0x04, 0x66, 0x6f, 0x72,
	0x6d, 0x18, 0x05, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x24, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
	0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x43, 0x6f,
	0x64, 0x65, 0x61, 0x62, 0x6c, 0x65, 0x43, 0x6f, 0x6e, 0x63, 0x65, 0x70, 0x74, 0x52, 0x04, 0x66,
	0x6f, 0x72, 0x6d, 0x3a, 0x44, 0xc0, 0x9f, 0xe3, 0xb6, 0x05, 0x03, 0xb2, 0xfe, 0xe4, 0x97, 0x06,
	0x38, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x68, 0x6c, 0x37, 0x2e, 0x6f, 0x72, 0x67, 0x2f,
	0x66, 0x68, 0x69, 0x72, 0x2f, 0x53, 0x74, 0x72, 0x75, 0x63, 0x74, 0x75, 0x72, 0x65, 0x44, 0x65,
	0x66, 0x69, 0x6e, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x2f, 0x45, 0x6e, 0x63, 0x6f, 0x75, 0x6e, 0x74,
	0x65, 0x72, 0x48, 0x69, 0x73, 0x74, 0x6f, 0x72, 0x79, 0x4a, 0x04, 0x08, 0x07, 0x10, 0x08, 0x42,
	0x81, 0x01, 0x98, 0xc6, 0xb0, 0xb5, 0x07, 0x05, 0x0a, 0x17, 0x63, 0x6f, 0x6d, 0x2e, 0x67, 0x6f,
	0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x66, 0x68, 0x69, 0x72, 0x2e, 0x72, 0x35, 0x2e, 0x63, 0x6f, 0x72,
	0x65, 0x50, 0x01, 0x5a, 0x5e, 0x67, 0x69, 0x74, 0x68, 0x75, 0x62, 0x2e, 0x63, 0x6f, 0x6d, 0x2f,
	0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2f, 0x66, 0x68, 0x69, 0x72, 0x2f, 0x67, 0x6f, 0x2f, 0x70,
	0x72, 0x6f, 0x74, 0x6f, 0x2f, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2f, 0x66, 0x68, 0x69, 0x72,
	0x2f, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x2f, 0x72, 0x35, 0x2f, 0x63, 0x6f, 0x72, 0x65, 0x2f, 0x72,
	0x65, 0x73, 0x6f, 0x75, 0x72, 0x63, 0x65, 0x73, 0x2f, 0x65, 0x6e, 0x63, 0x6f, 0x75, 0x6e, 0x74,
	0x65, 0x72, 0x5f, 0x68, 0x69, 0x73, 0x74, 0x6f, 0x72, 0x79, 0x5f, 0x67, 0x6f, 0x5f, 0x70, 0x72,
	0x6f, 0x74, 0x6f, 0x62, 0x06, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x33,
}

var (
	file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDescOnce sync.Once
	file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDescData = file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDesc
)

func file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDescGZIP() []byte {
	file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDescOnce.Do(func() {
		file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDescData = protoimpl.X.CompressGZIP(file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDescData)
	})
	return file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDescData
}

var file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_msgTypes = make([]protoimpl.MessageInfo, 3)
var file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_goTypes = []interface{}{
	(*EncounterHistory)(nil),                      // 0: google.fhir.r5.core.EncounterHistory
	(*EncounterHistory_StatusCode)(nil),           // 1: google.fhir.r5.core.EncounterHistory.StatusCode
	(*EncounterHistory_Location)(nil),             // 2: google.fhir.r5.core.EncounterHistory.Location
	(*datatypes_go_proto.Id)(nil),                 // 3: google.fhir.r5.core.Id
	(*datatypes_go_proto.Meta)(nil),               // 4: google.fhir.r5.core.Meta
	(*datatypes_go_proto.Uri)(nil),                // 5: google.fhir.r5.core.Uri
	(*datatypes_go_proto.Code)(nil),               // 6: google.fhir.r5.core.Code
	(*datatypes_go_proto.Narrative)(nil),          // 7: google.fhir.r5.core.Narrative
	(*anypb.Any)(nil),                             // 8: google.protobuf.Any
	(*datatypes_go_proto.Extension)(nil),          // 9: google.fhir.r5.core.Extension
	(*datatypes_go_proto.Reference)(nil),          // 10: google.fhir.r5.core.Reference
	(*datatypes_go_proto.Identifier)(nil),         // 11: google.fhir.r5.core.Identifier
	(*datatypes_go_proto.CodeableConcept)(nil),    // 12: google.fhir.r5.core.CodeableConcept
	(*datatypes_go_proto.CodeableReference)(nil),  // 13: google.fhir.r5.core.CodeableReference
	(*datatypes_go_proto.Period)(nil),             // 14: google.fhir.r5.core.Period
	(*datatypes_go_proto.DateTime)(nil),           // 15: google.fhir.r5.core.DateTime
	(*datatypes_go_proto.Duration)(nil),           // 16: google.fhir.r5.core.Duration
	(codes_go_proto.EncounterStatusCode_Value)(0), // 17: google.fhir.r5.core.EncounterStatusCode.Value
	(*datatypes_go_proto.String)(nil),             // 18: google.fhir.r5.core.String
}
var file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_depIdxs = []int32{
	3,  // 0: google.fhir.r5.core.EncounterHistory.id:type_name -> google.fhir.r5.core.Id
	4,  // 1: google.fhir.r5.core.EncounterHistory.meta:type_name -> google.fhir.r5.core.Meta
	5,  // 2: google.fhir.r5.core.EncounterHistory.implicit_rules:type_name -> google.fhir.r5.core.Uri
	6,  // 3: google.fhir.r5.core.EncounterHistory.language:type_name -> google.fhir.r5.core.Code
	7,  // 4: google.fhir.r5.core.EncounterHistory.text:type_name -> google.fhir.r5.core.Narrative
	8,  // 5: google.fhir.r5.core.EncounterHistory.contained:type_name -> google.protobuf.Any
	9,  // 6: google.fhir.r5.core.EncounterHistory.extension:type_name -> google.fhir.r5.core.Extension
	9,  // 7: google.fhir.r5.core.EncounterHistory.modifier_extension:type_name -> google.fhir.r5.core.Extension
	10, // 8: google.fhir.r5.core.EncounterHistory.encounter:type_name -> google.fhir.r5.core.Reference
	11, // 9: google.fhir.r5.core.EncounterHistory.identifier:type_name -> google.fhir.r5.core.Identifier
	1,  // 10: google.fhir.r5.core.EncounterHistory.status:type_name -> google.fhir.r5.core.EncounterHistory.StatusCode
	12, // 11: google.fhir.r5.core.EncounterHistory.class_value:type_name -> google.fhir.r5.core.CodeableConcept
	12, // 12: google.fhir.r5.core.EncounterHistory.type:type_name -> google.fhir.r5.core.CodeableConcept
	13, // 13: google.fhir.r5.core.EncounterHistory.service_type:type_name -> google.fhir.r5.core.CodeableReference
	10, // 14: google.fhir.r5.core.EncounterHistory.subject:type_name -> google.fhir.r5.core.Reference
	12, // 15: google.fhir.r5.core.EncounterHistory.subject_status:type_name -> google.fhir.r5.core.CodeableConcept
	14, // 16: google.fhir.r5.core.EncounterHistory.actual_period:type_name -> google.fhir.r5.core.Period
	15, // 17: google.fhir.r5.core.EncounterHistory.planned_start_date:type_name -> google.fhir.r5.core.DateTime
	15, // 18: google.fhir.r5.core.EncounterHistory.planned_end_date:type_name -> google.fhir.r5.core.DateTime
	16, // 19: google.fhir.r5.core.EncounterHistory.length:type_name -> google.fhir.r5.core.Duration
	2,  // 20: google.fhir.r5.core.EncounterHistory.location:type_name -> google.fhir.r5.core.EncounterHistory.Location
	17, // 21: google.fhir.r5.core.EncounterHistory.StatusCode.value:type_name -> google.fhir.r5.core.EncounterStatusCode.Value
	18, // 22: google.fhir.r5.core.EncounterHistory.StatusCode.id:type_name -> google.fhir.r5.core.String
	9,  // 23: google.fhir.r5.core.EncounterHistory.StatusCode.extension:type_name -> google.fhir.r5.core.Extension
	18, // 24: google.fhir.r5.core.EncounterHistory.Location.id:type_name -> google.fhir.r5.core.String
	9,  // 25: google.fhir.r5.core.EncounterHistory.Location.extension:type_name -> google.fhir.r5.core.Extension
	9,  // 26: google.fhir.r5.core.EncounterHistory.Location.modifier_extension:type_name -> google.fhir.r5.core.Extension
	10, // 27: google.fhir.r5.core.EncounterHistory.Location.location:type_name -> google.fhir.r5.core.Reference
	12, // 28: google.fhir.r5.core.EncounterHistory.Location.form:type_name -> google.fhir.r5.core.CodeableConcept
	29, // [29:29] is the sub-list for method output_type
	29, // [29:29] is the sub-list for method input_type
	29, // [29:29] is the sub-list for extension type_name
	29, // [29:29] is the sub-list for extension extendee
	0,  // [0:29] is the sub-list for field type_name
}

func init() { file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_init() }
func file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_init() {
	if File_proto_google_fhir_proto_r5_core_resources_encounter_history_proto != nil {
		return
	}
	if !protoimpl.UnsafeEnabled {
		file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*EncounterHistory); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_msgTypes[1].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*EncounterHistory_StatusCode); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_msgTypes[2].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*EncounterHistory_Location); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
	}
	type x struct{}
	out := protoimpl.TypeBuilder{
		File: protoimpl.DescBuilder{
			GoPackagePath: reflect.TypeOf(x{}).PkgPath(),
			RawDescriptor: file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDesc,
			NumEnums:      0,
			NumMessages:   3,
			NumExtensions: 0,
			NumServices:   0,
		},
		GoTypes:           file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_goTypes,
		DependencyIndexes: file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_depIdxs,
		MessageInfos:      file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_msgTypes,
	}.Build()
	File_proto_google_fhir_proto_r5_core_resources_encounter_history_proto = out.File
	file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_rawDesc = nil
	file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_goTypes = nil
	file_proto_google_fhir_proto_r5_core_resources_encounter_history_proto_depIdxs = nil
}