grammar Cql;

/*
 * Clinical Quality Language Grammar Specification
 * Version 1.5 - Mixed Normative/Trial-Use
 */

import FhirPath;

/*
 * Parser Rules
 */

definition
    : usingDefinition
    | includeDefinition
    | codesystemDefinition
    | valuesetDefinition
    | codeDefinition
    | conceptDefinition
    | parameterDefinition
    ;

library
    :
    libraryDefinition?
    definition*
    statement*
    EOF
    ;

/*
 * Definitions
 */

libraryDefinition
    : 'library' qualifiedIdentifier ('version' versionSpecifier)?
    ;

usingDefinition
    : 'using' modelIdentifier ('version' versionSpecifier)?
    ;

includeDefinition
    : 'include' qualifiedIdentifier ('version' versionSpecifier)? ('called' localIdentifier)?
    ;

localIdentifier
    : identifier
    ;

accessModifier
    : 'public'
    | 'private'
    ;

parameterDefinition
    : accessModifier? 'parameter' identifier (typeSpecifier)? ('default' expression)?
    ;

codesystemDefinition
    : accessModifier? 'codesystem' identifier ':' codesystemId ('version' versionSpecifier)?
    ;

valuesetDefinition
    : accessModifier? 'valueset' identifier ':' valuesetId ('version' versionSpecifier)? codesystems?
    ;

codesystems
    : 'codesystems' '{' codesystemIdentifier (',' codesystemIdentifier)* '}'
    ;

codesystemIdentifier
    : (libraryIdentifier '.')? identifier
    ;

libraryIdentifier
    : identifier
    ;

codeDefinition
    : accessModifier? 'code' identifier ':' codeId 'from' codesystemIdentifier displayClause?
    ;

conceptDefinition
    : accessModifier? 'concept' identifier ':' '{' codeIdentifier (',' codeIdentifier)* '}' displayClause?
    ;

codeIdentifier
    : (libraryIdentifier '.')? identifier
    ;

codesystemId
    : STRING
    ;

valuesetId
    : STRING
    ;

versionSpecifier
    : STRING
    ;

codeId
    : STRING
    ;

/*
 * Type Specifiers
 */

typeSpecifier
    : namedTypeSpecifier
    | listTypeSpecifier
    | intervalTypeSpecifier
    | tupleTypeSpecifier
    | choiceTypeSpecifier
    ;

namedTypeSpecifier
    : (qualifier '.')* referentialOrTypeNameIdentifier
    ;

modelIdentifier
    : identifier
    ;

listTypeSpecifier
    : 'List' '<' typeSpecifier '>'
    ;

intervalTypeSpecifier
    : 'Interval' '<' typeSpecifier '>'
    ;

tupleTypeSpecifier
    : 'Tuple' '{' tupleElementDefinition (',' tupleElementDefinition)* '}'
    ;

tupleElementDefinition
    : referentialIdentifier typeSpecifier
    ;

choiceTypeSpecifier
    : 'Choice' '<' typeSpecifier (',' typeSpecifier)* '>'
    ;

/*
 * Statements
 */

statement
    : expressionDefinition
    | contextDefinition
    | functionDefinition
    ;

expressionDefinition
    : 'define' accessModifier? identifier ':' expression
    ;

contextDefinition
    : 'context' (modelIdentifier '.')? identifier
    ;

functionDefinition
    : 'define' accessModifier? 'fluent'? 'function' identifierOrFunctionIdentifier '(' (operandDefinition (',' operandDefinition)*)? ')'
        ('returns' typeSpecifier)?
        ':' (functionBody | 'external')
    ;

operandDefinition
    : referentialIdentifier typeSpecifier
    ;

functionBody
    : expression
    ;

/*
 * Expressions
 */

querySource
    : retrieve
    | qualifiedIdentifierExpression
    | '(' expression ')'
    ;

aliasedQuerySource
    : querySource alias
    ;

alias
    : identifier
    ;

queryInclusionClause
    : withClause
    | withoutClause
    ;

withClause
    : 'with' aliasedQuerySource 'such that' expression
    ;

withoutClause
    : 'without' aliasedQuerySource 'such that' expression
    ;

retrieve
    : '[' (contextIdentifier '->')? namedTypeSpecifier (':' (codePath codeComparator)? terminology)? ']'
    ;

contextIdentifier
    : qualifiedIdentifierExpression
    ;

codePath
    : simplePath
    ;

codeComparator
    : 'in'
    | '='
    | '~'
    ;

terminology
    : qualifiedIdentifierExpression
    | expression
    ;

qualifier
    : identifier
    ;

query
    : sourceClause letClause? queryInclusionClause* whereClause? (aggregateClause | returnClause)? sortClause?
    ;

sourceClause
    : 'from'? aliasedQuerySource (',' aliasedQuerySource)*
    ;

letClause
    : 'let' letClauseItem (',' letClauseItem)*
    ;

letClauseItem
    : identifier ':' expression
    ;

whereClause
    : 'where' expression
    ;

returnClause
    : 'return' ('all' | 'distinct')? expression
    ;

aggregateClause
    : 'aggregate' ('all' | 'distinct')? identifier startingClause? ':' expression
    ;

startingClause
    : 'starting' (simpleLiteral | quantity | ('(' expression ')'))
    ;

sortClause
    : 'sort' ( sortDirection | ('by' sortByItem (',' sortByItem)*) )
    ;

sortDirection
    : 'asc' | 'ascending'
    | 'desc' | 'descending'
    ;

sortByItem
    : expressionTerm sortDirection?
    ;

qualifiedIdentifier
    : (qualifier '.')* identifier
    ;

qualifiedIdentifierExpression
    : (qualifierExpression '.')* referentialIdentifier
    ;

qualifierExpression
    : referentialIdentifier
    ;

simplePath
    : referentialIdentifier                            #simplePathReferentialIdentifier
    | simplePath '.' referentialIdentifier             #simplePathQualifiedIdentifier
    | simplePath '[' simpleLiteral ']'                 #simplePathIndexer
    ;

simpleLiteral
    : STRING                                           #simpleStringLiteral
    | NUMBER                                           #simpleNumberLiteral
    ;

expression
    : expressionTerm                                                                                #termExpression
    | retrieve                                                                                      #retrieveExpression
    | query                                                                                         #queryExpression
    | expression 'is' 'not'? ('null' | 'true' | 'false')                                            #booleanExpression
    | expression ('is' | 'as') typeSpecifier                                                        #typeExpression
    | 'cast' expression 'as' typeSpecifier                                                          #castExpression
    | 'not' expression                                                                              #notExpression
    | 'exists' expression                                                                           #existenceExpression
    | expression 'properly'? 'between' expressionTerm 'and' expressionTerm                          #betweenExpression
    | ('duration' 'in')? pluralDateTimePrecision 'between' expressionTerm 'and' expressionTerm      #durationBetweenExpression
    | 'difference' 'in' pluralDateTimePrecision 'between' expressionTerm 'and' expressionTerm       #differenceBetweenExpression
    | expression ('<=' | '<' | '>' | '>=') expression                                               #inequalityExpression
    | expression intervalOperatorPhrase expression                                                  #timingExpression
    | expression ('=' | '!=' | '~' | '!~') expression                                               #equalityExpression
    | expression ('in' | 'contains') dateTimePrecisionSpecifier? expression                         #membershipExpression
    | expression 'and' expression                                                                   #andExpression
    | expression ('or' | 'xor') expression                                                          #orExpression
    | expression 'implies' expression                                                               #impliesExpression
    | expression ('|' | 'union' | 'intersect' | 'except') expression                                #inFixSetExpression
    ;

dateTimePrecision
    : 'year' | 'month' | 'week' | 'day' | 'hour' | 'minute' | 'second' | 'millisecond'
    ;

dateTimeComponent
    : dateTimePrecision
    | 'date'
    | 'time'
    | 'timezoneoffset'
    ;

pluralDateTimePrecision
    : 'years' | 'months' | 'weeks' | 'days' | 'hours' | 'minutes' | 'seconds' | 'milliseconds'
    ;

expressionTerm
    : term                                                                          #termExpressionTerm
    | expressionTerm '.' qualifiedInvocation                                        #invocationExpressionTerm
    | expressionTerm '[' expression ']'                                             #indexedExpressionTerm
    | 'convert' expression 'to' (typeSpecifier | unit)                              #conversionExpressionTerm
    | ('+' | '-') expressionTerm                                                    #polarityExpressionTerm
    | ('start' | 'end') 'of' expressionTerm                                         #timeBoundaryExpressionTerm
    | dateTimeComponent 'from' expressionTerm                                       #timeUnitExpressionTerm
    | 'duration' 'in' pluralDateTimePrecision 'of' expressionTerm                   #durationExpressionTerm
    | 'difference' 'in' pluralDateTimePrecision 'of' expressionTerm                 #differenceExpressionTerm
    | 'width' 'of' expressionTerm                                                   #widthExpressionTerm
    | 'successor' 'of' expressionTerm                                               #successorExpressionTerm
    | 'predecessor' 'of' expressionTerm                                             #predecessorExpressionTerm
    | 'singleton' 'from' expressionTerm                                             #elementExtractorExpressionTerm
    | 'point' 'from' expressionTerm                                                 #pointExtractorExpressionTerm
    | ('minimum' | 'maximum') namedTypeSpecifier                                    #typeExtentExpressionTerm
    | expressionTerm '^' expressionTerm                                             #powerExpressionTerm
    | expressionTerm ('*' | '/' | 'div' | 'mod') expressionTerm                     #multiplicationExpressionTerm
    | expressionTerm ('+' | '-' | '&') expressionTerm                               #additionExpressionTerm
    | 'if' expression 'then' expression 'else' expression                           #ifThenElseExpressionTerm
    | 'case' expression? caseExpressionItem+ 'else' expression 'end'                #caseExpressionTerm
    | ('distinct' | 'flatten') expression                                           #aggregateExpressionTerm
    | ('expand' | 'collapse') expression ('per' (dateTimePrecision | expression))?  #setAggregateExpressionTerm
    ;

caseExpressionItem
    : 'when' expression 'then' expression
    ;

dateTimePrecisionSpecifier
    : dateTimePrecision 'of'
    ;

relativeQualifier
    : 'or before'
    | 'or after'
    ;

offsetRelativeQualifier
    : 'or more'
    | 'or less'
    ;

exclusiveRelativeQualifier
    : 'less than'
    | 'more than'
    ;

quantityOffset
    : (quantity offsetRelativeQualifier?)
    | (exclusiveRelativeQualifier quantity)
    ;

temporalRelationship
    : ('on or'? ('before' | 'after'))
    | (('before' | 'after') 'or on'?)
    ;

intervalOperatorPhrase
    : ('starts' | 'ends' | 'occurs')? 'same' dateTimePrecision? (relativeQualifier | 'as') ('start' | 'end')?               #concurrentWithIntervalOperatorPhrase
    | 'properly'? 'includes' dateTimePrecisionSpecifier? ('start' | 'end')?                                                 #includesIntervalOperatorPhrase
    | ('starts' | 'ends' | 'occurs')? 'properly'? ('during' | 'included in') dateTimePrecisionSpecifier?                    #includedInIntervalOperatorPhrase
    | ('starts' | 'ends' | 'occurs')? quantityOffset? temporalRelationship dateTimePrecisionSpecifier? ('start' | 'end')?   #beforeOrAfterIntervalOperatorPhrase
    | ('starts' | 'ends' | 'occurs')? 'properly'? 'within' quantity 'of' ('start' | 'end')?                                 #withinIntervalOperatorPhrase
    | 'meets' ('before' | 'after')? dateTimePrecisionSpecifier?                                                             #meetsIntervalOperatorPhrase
    | 'overlaps' ('before' | 'after')? dateTimePrecisionSpecifier?                                                          #overlapsIntervalOperatorPhrase
    | 'starts' dateTimePrecisionSpecifier?                                                                                  #startsIntervalOperatorPhrase
    | 'ends' dateTimePrecisionSpecifier?                                                                                    #endsIntervalOperatorPhrase
    ;

term
    : invocation            #invocationTerm
    | literal               #literalTerm
    | externalConstant      #externalConstantTerm
    | intervalSelector      #intervalSelectorTerm
    | tupleSelector         #tupleSelectorTerm
    | instanceSelector      #instanceSelectorTerm
    | listSelector          #listSelectorTerm
    | codeSelector          #codeSelectorTerm
    | conceptSelector       #conceptSelectorTerm
    | '(' expression ')'    #parenthesizedTerm
    ;

qualifiedInvocation // Terms that can be used after the function/member invocation '.'
    : referentialIdentifier             #qualifiedMemberInvocation
    | qualifiedFunction                 #qualifiedFunctionInvocation
    ;

qualifiedFunction
    : identifierOrFunctionIdentifier '(' paramList? ')'
    ;

invocation
    : referentialIdentifier             #memberInvocation
    | function                          #functionInvocation
    | '$this'                           #thisInvocation
    | '$index'                          #indexInvocation
    | '$total'                          #totalInvocation
    ;

function
    : referentialIdentifier '(' paramList? ')'
    ;

ratio
    : quantity ':' quantity
    ;

literal
    : ('true' | 'false')                                    #booleanLiteral
    | 'null'                                                #nullLiteral
    | STRING                                                #stringLiteral
    | NUMBER                                                #numberLiteral
    | LONGNUMBER                                            #longNumberLiteral
    | DATETIME                                              #dateTimeLiteral
    | DATE                                                  #dateLiteral
    | TIME                                                  #timeLiteral
    | quantity                                              #quantityLiteral
    | ratio                                                 #ratioLiteral
    ;

intervalSelector
    : // TODO: Consider this as an alternative syntax for intervals... (would need to be moved up to expression to make it work)
    //expression ( '..' | '*.' | '.*' | '**' ) expression;
    'Interval' ('['|'(') expression ',' expression (']'|')')
    ;

tupleSelector
    : 'Tuple'? '{' (':' | (tupleElementSelector (',' tupleElementSelector)*)) '}'
    ;

tupleElementSelector
    : referentialIdentifier ':' expression
    ;

instanceSelector
    : namedTypeSpecifier '{' (':' | (instanceElementSelector (',' instanceElementSelector)*)) '}'
    ;

instanceElementSelector
    : referentialIdentifier ':' expression
    ;

listSelector
    : ('List' ('<' typeSpecifier '>')?)? '{' (expression (',' expression)*)? '}'
    ;

displayClause
    : 'display' STRING
    ;

codeSelector
    : 'Code' STRING 'from' codesystemIdentifier displayClause?
    ;

conceptSelector
    : 'Concept' '{' codeSelector (',' codeSelector)* '}' displayClause?
    ;

keyword
    : 'after'
    | 'aggregate'
    | 'all'
    | 'and'
    | 'as'
    | 'asc'
    | 'ascending'
    | 'before'
    | 'between'
    | 'by'
    | 'called'
    | 'case'
    | 'cast'
    | 'code'
    | 'Code'
    | 'codesystem'
    | 'codesystems'
    | 'collapse'
    | 'concept'
    | 'Concept'
    | 'contains'
    | 'context'
    | 'convert'
    | 'date'
    | 'day'
    | 'days'
    | 'default'
    | 'define'
    | 'desc'
    | 'descending'
    | 'difference'
    | 'display'
    | 'distinct'
    | 'div'
    | 'duration'
    | 'during'
    | 'else'
    | 'end'
    | 'ends'
    | 'except'
    | 'exists'
    | 'expand'
    | 'false'
    | 'flatten'
    | 'fluent'
    | 'from'
    | 'function'
    | 'hour'
    | 'hours'
    | 'if'
    | 'implies'
    | 'in'
    | 'include'
    | 'includes'
    | 'included in'
    | 'intersect'
    | 'Interval'
    | 'is'
    | 'let'
    | 'library'
    | 'List'
    | 'maximum'
    | 'meets'
    | 'millisecond'
    | 'milliseconds'
    | 'minimum'
    | 'minute'
    | 'minutes'
    | 'mod'
    | 'month'
    | 'months'
    | 'not'
    | 'null'
    | 'occurs'
    | 'of'
    | 'on or'
    | 'or'
    | 'or after'
    | 'or before'
    | 'or less'
    | 'or more'
    | 'or on'
    | 'overlaps'
    | 'parameter'
    | 'per'
    | 'point'
    | 'predecessor'
    | 'private'
    | 'properly'
    | 'public'
    | 'return'
    | 'same'
    | 'second'
    | 'seconds'
    | 'singleton'
    | 'start'
    | 'starting'
    | 'starts'
    | 'sort'
    | 'successor'
    | 'such that'
    | 'then'
    | 'time'
    | 'timezoneoffset'
    | 'to'
    | 'true'
    | 'Tuple'
    | 'union'
    | 'using'
    | 'valueset'
    | 'version'
    | 'week'
    | 'weeks'
    | 'where'
    | 'when'
    | 'width'
    | 'with'
    | 'within'
    | 'without'
    | 'xor'
    | 'year'
    | 'years'
    ;

// NOTE: Not used, this is the set of reserved words that may not appear as identifiers in ambiguous contexts
reservedWord
    : 'aggregate'
    | 'all'
    | 'and'
    | 'as'
    | 'after'
    | 'before'
    | 'between'
    | 'case'
    | 'cast'
    | 'Code'
    | 'collapse'
    | 'Concept'
    | 'convert'
    | 'day'
    | 'days'
    | 'difference'
    | 'distinct'
    | 'duration'
    | 'during'
    | 'else'
    | 'exists'
    | 'expand'
    | 'false'
    | 'flatten'
    | 'from'
    | 'if'
    | 'in'
    | 'included in'
    | 'is'
    | 'hour'
    | 'hours'
    | 'Interval'
    | 'let'
    | 'List'
    | 'maximum'
    | 'millisecond'
    | 'milliseconds'
    | 'minimum'
    | 'minute'
    | 'minutes'
    | 'month'
    | 'months'
    | 'not'
    | 'null'
    | 'occurs'
    | 'of'
    | 'on or'
    | 'or'
    | 'or on'
    | 'per'
    | 'point'
    | 'properly'
    | 'return'
    | 'same'
    | 'second'
    | 'seconds'
    | 'singleton'
    | 'sort'
    | 'such that'
    | 'then'
    | 'to'
    | 'true'
    | 'Tuple'
    | 'week'
    | 'weeks'
    | 'when'
    | 'with'
    | 'within'
    | 'without'
    | 'year'
    | 'years'
    ;

// Keyword identifiers are keywords that may be used as identifiers in a referential context
// Effectively, keyword except reservedWord
keywordIdentifier
    : 'asc'
    | 'ascending'
    | 'by'
    | 'called'
    | 'code'
    | 'codesystem'
    | 'codesystems'
    | 'concept'
    | 'contains'
    | 'context'
    | 'date'
    | 'default'
    | 'define'
    | 'desc'
    | 'descending'
    | 'display'
    | 'div'
    | 'end'
    | 'ends'
    | 'except'
    | 'fluent'
    | 'function'
    | 'implies'
    | 'include'
    | 'includes'
    | 'intersect'
    | 'library'
    | 'meets'
    | 'mod'
    | 'or after'
    | 'or before'
    | 'or less'
    | 'or more'
    | 'overlaps'
    | 'parameter'
    | 'predecessor'
    | 'private'
    | 'public'
    | 'start'
    | 'starting'
    | 'starts'
    | 'successor'
    | 'time'
    | 'timezoneoffset'
    | 'union'
    | 'using'
    | 'valueset'
    | 'version'
    | 'where'
    | 'width'
    | 'xor'
    ;

// Obsolete identifiers are keywords that could be used as identifiers in CQL 1.3
// NOTE: Not currently used, this is the set of keywords that were defined as allowed identifiers as part of 1.3
// NOTE: Several keywords were commented out in this list (notably exists) because of an issue with the ANTLR tooling.
// In 4.5, having these keywords as identifiers results in unacceptable parsing performance. In 4.6+, having them as
// identifiers resulted in incorrect parsing. See Github issue [#343](https://github.com/cqframework/clinical_quality_language/issues/343) for more detail
// This should no longer be an issue with 1.4 due to the introduction of reserved words
obsoleteIdentifier
    : 'all'
    | 'Code'
    | 'code'
    | 'Concept'
    | 'concept'
    | 'contains'
    | 'date'
    | 'display'
    | 'distinct'
    | 'end'
    | 'exists'
    | 'not'
    | 'start'
    | 'time'
    | 'timezoneoffset'
    | 'version'
    | 'where'
    ;

// Function identifiers are keywords that may be used as identifiers for functions
functionIdentifier
    : 'after'
    | 'aggregate'
    | 'all'
    | 'and'
    | 'as'
    | 'asc'
    | 'ascending'
    | 'before'
    | 'between'
    | 'by'
    | 'called'
    | 'case'
    | 'cast'
    | 'code'
    | 'Code'
    | 'codesystem'
    | 'codesystems'
    | 'collapse'
    | 'concept'
    | 'Concept'
    | 'contains'
    | 'context'
    | 'convert'
    | 'date'
    | 'day'
    | 'days'
    | 'default'
    | 'define'
    | 'desc'
    | 'descending'
    | 'difference'
    | 'display'
    | 'distinct'
    | 'div'
    | 'duration'
    | 'during'
    | 'else'
    | 'end'
    | 'ends'
    | 'except'
    | 'exists'
    | 'expand'
    | 'false'
    | 'flatten'
    | 'fluent'
    | 'from'
    | 'function'
    | 'hour'
    | 'hours'
    | 'if'
    | 'implies'
    | 'in'
    | 'include'
    | 'includes'
    | 'included in'
    | 'intersect'
    | 'Interval'
    | 'is'
    | 'let'
    | 'library'
    | 'List'
    | 'maximum'
    | 'meets'
    | 'millisecond'
    | 'milliseconds'
    | 'minimum'
    | 'minute'
    | 'minutes'
    | 'mod'
    | 'month'
    | 'months'
    | 'not'
    | 'null'
    | 'occurs'
    | 'of'
    | 'or'
    | 'or after'
    | 'or before'
    | 'or less'
    | 'or more'
    | 'overlaps'
    | 'parameter'
    | 'per'
    | 'point'
    | 'predecessor'
    | 'private'
    | 'properly'
    | 'public'
    | 'return'
    | 'same'
    | 'singleton'
    | 'second'
    | 'seconds'
    | 'start'
    | 'starting'
    | 'starts'
    | 'sort'
    | 'successor'
    | 'such that'
    | 'then'
    | 'time'
    | 'timezoneoffset'
    | 'to'
    | 'true'
    | 'Tuple'
    | 'union'
    | 'using'
    | 'valueset'
    | 'version'
    | 'week'
    | 'weeks'
    | 'where'
    | 'when'
    | 'width'
    | 'with'
    | 'within'
    | 'without'
    | 'xor'
    | 'year'
    | 'years'
    ;

// Reserved words that are also type names
typeNameIdentifier
    : 'Code'
    | 'Concept'
    | 'date'
    | 'time'
    ;

referentialIdentifier
    : identifier
    | keywordIdentifier
    ;

referentialOrTypeNameIdentifier
    : referentialIdentifier
    | typeNameIdentifier
    ;

identifierOrFunctionIdentifier
    : identifier
    | functionIdentifier
    ;

identifier
    : IDENTIFIER
    | DELIMITEDIDENTIFIER
    | QUOTEDIDENTIFIER
    ;

QUOTEDIDENTIFIER
    : '"' (ESC | .)*? '"'
    ;

DATETIME
    : '@' DATEFORMAT 'T' TIMEFORMAT? TIMEZONEOFFSETFORMAT?
    ;

LONGNUMBER
    : [0-9]+'L'
    ;

fragment ESC
    : '\\' ([`'"\\/fnrt] | UNICODE)    // allow \`, \', \", \\, \/, \f, etc. and \uXXX
    ;
