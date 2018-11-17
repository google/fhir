/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package json

import (
	"github.com/golang/protobuf/jsonpb"
	pb "github.com/google/fhir/proto/stu3"
)

// Compile-time checks that each primitive-type proto satisfies the necessary
// interfaces for JSON marshalling.
var (
	_ jsonpb.JSONPBMarshaler   = (*pb.Base64Binary)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Base64Binary)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.Boolean)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Boolean)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.Code)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Code)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.Date)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Date)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.DateTime)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.DateTime)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.Decimal)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Decimal)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.Id)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Id)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.Instant)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Instant)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.Integer)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Integer)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.Markdown)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Markdown)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.Oid)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Oid)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.PositiveInt)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.PositiveInt)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.String)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.String)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.Time)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Time)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.UnsignedInt)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.UnsignedInt)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.Uri)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Uri)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.Uuid)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Uuid)(nil)
	_ jsonpb.JSONPBMarshaler   = (*pb.Xhtml)(nil)
	_ jsonpb.JSONPBUnmarshaler = (*pb.Xhtml)(nil)
)
