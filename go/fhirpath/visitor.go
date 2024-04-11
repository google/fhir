// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package fhirpath

import (
	"errors"
	"fmt"

	"github.com/google/fhir/go/fhirpath/gen"
	"github.com/antlr4-go/antlr/v4"
)

var (
	_ gen.FhirPathVisitor = (*compilerVisitor)(nil)
	_ antlr.ErrorListener = (*errorListener)(nil)
)

// compilerVisitor is a ANTLR Visitor implementation.
type compilerVisitor struct {
	*gen.BaseFhirPathVisitor

	errorListener *errorListener
	err           error
}

// SetError implements FhirPathVisitor
func (v *compilerVisitor) SetError(err error) {
	v.err = err
}

// Err implements FhirPathVisitor
func (v *compilerVisitor) Err() error {
	return v.err
}

// Visit implements FhirPathVisitor
func (v *compilerVisitor) Visit(tree antlr.ParseTree) any {
	v.err = fmt.Errorf("unimplemented")
	return nil
}

// VisitErrorNode implements FhirPathVisitor
func (v *compilerVisitor) VisitErrorNode(node antlr.ErrorNode) any {
	v.SetError(fmt.Errorf("unknown error encountered: %s", node.GetText()))
	return nil
}

// errorListener is an ANTLR listener to report syntax errors.
type errorListener struct {
	*antlr.DefaultErrorListener

	visitor *compilerVisitor
}

// SyntaxError implements antlr.ErrorListener.
func (l *errorListener) SyntaxError(recognizer antlr.Recognizer, offendingSymbol any, line, column int, msg string, e antlr.RecognitionException) {
	l.visitor.SetError(errors.New(msg))
}
