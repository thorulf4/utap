// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
///////////////////////////////////////////////////////////////////////////////
//
// This file is a part of UPPAAL.
// Copyright (c) 2021-2022, Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////////////////

/**
 * File:   test_parser.cpp
 * Author: Rasmus R. Kjær
 *
 * Created on 20 August 2021, 09:47
 */

#include "document_fixture.h"

#include "utap/StatementBuilder.hpp"
#include "utap/typechecker.h"
#include "utap/utap.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

inline std::string read_content(const std::string& file_name)
{
    const auto path = std::filesystem::path{MODELS_DIR} / file_name;
    auto ifs = std::ifstream{path};
    if (ifs.fail())
        throw std::system_error{errno, std::system_category(), "Failed to open " + path.string()};
    auto content = std::string{std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{}};
    REQUIRE(!content.empty());
    return content;
}

std::unique_ptr<UTAP::Document> read_document(const std::string& file_name)
{
    auto doc = std::make_unique<UTAP::Document>();
    auto res = parse_XML_buffer(read_content(file_name).c_str(), doc.get(), true);
    REQUIRE(res == 0);
    return doc;
}

TEST_CASE("Double Serialization Test")
{
    auto doc = read_document("if_statement.xml");
    REQUIRE(doc);
    auto& errors = doc->get_errors();
    CHECK(errors.size() == 0);
    auto& warnings = doc->get_warnings();
    CHECK(warnings.size() == 0);
}

TEST_CASE("Power expressions")
{
    auto doc = read_document("powers.xml");
    REQUIRE(doc);
    auto& errors = doc->get_errors();
    CHECK(errors.size() == 0);
    auto& warnings = doc->get_warnings();
    CHECK(warnings.size() == 0);
}

TEST_CASE("External functions")
{
    auto doc = read_document("external_fn.xml");
    REQUIRE(doc);
    auto& errs = doc->get_errors();
    auto& warns = doc->get_warnings();
    REQUIRE(errs.size() == 3);  // "libbad" not found (x2), "absent" undefined.
    CHECK(warns.size() == 0);
}

TEST_CASE("Error location")
{
    auto doc = read_document("smc_non-deterministic_input2.xml");
    REQUIRE(doc);
    auto& errs = doc->get_errors();
    auto& warns = doc->get_warnings();
    REQUIRE(errs.size() == 0);
    CHECK(warns.size() == 0);
    const auto& templates = doc->get_templates();
    REQUIRE(templates.size() > 0);
    const auto& edges = templates.front().edges;
    REQUIRE(edges.size() > 0);
    const auto& pos = edges.front().sync.get_position();
    doc->add_error(pos, "Non-deterministic input", "c?");
    REQUIRE(errs.size() == 1);
    const auto& error = errs.front();
    REQUIRE(error.start.path != nullptr);
    CHECK(*error.start.path == "/nta/template[1]/transition[1]/label[1]");
}

class QueryBuilder : public UTAP::StatementBuilder
{
    UTAP::expression_t query;
    UTAP::TypeChecker checker;

public:
    explicit QueryBuilder(UTAP::Document& doc): UTAP::StatementBuilder{doc, doc.get_globals().frame}, checker{doc} {}
    void property() override
    {
        REQUIRE(fragments.size() > 0);
        query = fragments[0];
        fragments.pop();
    }
    void typecheck() { checker.checkExpression(query); }
    [[nodiscard]] UTAP::expression_t getQuery() const { return query; }
    UTAP::variable_t* addVariable(UTAP::type_t type, const std::string& name, UTAP::expression_t init,
                                  UTAP::position_t pos) override
    {
        throw UTAP::NotSupportedException("addVariable is not supported");
    }
    bool addFunction(UTAP::type_t type, const std::string& name, UTAP::position_t pos) override
    {
        throw UTAP::NotSupportedException("addFunction is not supported");
    }
};

TEST_CASE("SMC bounds in queries")
{
    auto doc = std::make_unique<UTAP::Document>();
    auto builder = std::make_unique<QueryBuilder>(*doc);
    SUBCASE("Probability estimation query with 7 runs")
    {
        auto res = parseProperty("Pr[<=1;7](<> true)", builder.get());
        REQUIRE(res == 0);
        auto expr = builder->getQuery();
        REQUIRE(expr.get_size() == 5);
        CHECK(expr.get(0).get_value() == 7);  // number of runs
    }
    SUBCASE("Probability estimation query without runs")
    {
        auto res = parseProperty("Pr[<=1](<> true)", builder.get());
        REQUIRE(res == 0);
        auto expr = builder->getQuery();
        REQUIRE(expr.get_size() == 5);
        CHECK(expr.get(0).get_value() == -1);  // number of runs
    }
    SUBCASE("Value estimation query with 7 runs")
    {
        auto res = parseProperty("E[<=1;7](max: 1)", builder.get());
        REQUIRE(res == 0);
        auto expr = builder->getQuery();
        REQUIRE(expr.get_size() == 5);
        CHECK(expr.get(0).get_value() == 7);  // number of runs
    }
    SUBCASE("Value estimation query without runs")
    {
        auto res = parseProperty("E[<=1](max: 1)", builder.get());
        REQUIRE(res == 0);
        auto expr = builder->getQuery();
        REQUIRE(expr.get_size() == 5);
        CHECK(expr.get(0).get_value() == -1);  // number of runs
    }
}

TEST_CASE("Parsing implicit goals for learning queries")
{
    using UTAP::Constants::kind_t;
    auto doc = read_document("simpleSystem.xml");
    auto builder = std::make_unique<QueryBuilder>(*doc);

    SUBCASE("Implicit constraint goal")
    {
        auto res = parseProperty("minE[c<=25]", builder.get());
        REQUIRE(res == 0);
        builder->typecheck();
        REQUIRE(doc->get_errors().size() == 0);
    }

    SUBCASE("Implicit time goal")
    {
        auto res = parseProperty("minE[<=20]", builder.get());
        REQUIRE(res == 0);
        builder->typecheck();
        REQUIRE(doc->get_errors().size() == 0);
    }

    SUBCASE("Implicit step goal")
    {
        REQUIRE(doc->get_errors().size() == 0);
        auto res = parseProperty("minE[#<=20]", builder.get());
        REQUIRE(res == 0);
        builder->typecheck();
        REQUIRE(doc->get_errors().size() == 0);
    }
}

TEST_CASE("Test builtin-, global-, system-declarations structure")
{
    // We expect the following frame structure
    // - Builtin declarations
    //     - Global declarations
    //         - Templates
    //         - System declarations

    std::unique_ptr<UTAP::Document> doc = read_document("simpleSystem.xml");
    UTAP::frame_t frame = doc->get_globals().frame;
    UTAP::frame_t sys_frame = doc->get_system_declarations().frame;
    CHECK(frame.get_size() == 6);
    CHECK(frame.has_parent());

    for (UTAP::template_t& templ : doc->get_templates())
        CHECK((templ.frame.get_parent() == frame) == true);

    REQUIRE(sys_frame.has_parent());
    CHECK((sys_frame.get_parent() == frame) == true);
}

TEST_CASE("Heap-use-after-free reported by ASAN due to double free")
{
    auto f = document_fixture{};
    f.add_global_decl("void f(){ int x = }");

    auto doc = f.parse();
}