// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2020 Aalborg University.
   Copyright (C) 2002-2006 Uppsala University and Aalborg University.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA
*/

#ifndef UTAP_INTERMEDIATE_HH
#define UTAP_INTERMEDIATE_HH

#include "utap/dynlib.h"
#include "utap/expression.h"
#include "utap/position.h"
#include "utap/symbols.h"

#include <algorithm>  // find
#include <deque>
#include <list>
#include <map>
#include <optional>
#include <vector>

namespace UTAP {
// Describes supported analysis methods for the given document
struct SupportedMethods
{
    bool symbolic{true};
    bool stochastic{true};
    bool concrete{true};
};

/** Adds str() method which calls Item::print(std::ostream&) to produce a string representation */
template <typename Item>
struct stringify_t
{
    std::string str() const;
};

/** Adds str(indent) method which calls Item::print(std::ostream&, indent) to produce a string representation */
template <typename Item>
struct stringify_indent_t : stringify_t<Item>
{
    std::string str(const std::string& indent) const;
};

/** Base type for variables, clocks, etc.  The user data of the
    corresponding symbol_t points to this structure,
    i.e. v.uid.get_data() is a pointer to v.
*/
struct variable_t : stringify_t<variable_t>
{
    symbol_t uid;                             /**< The symbol of the variable */
    expression_t init;                        /**< The initializer */
    std::ostream& print(std::ostream&) const; /**< outputs a textual representation */
};

/** Information about a location.
    The symbol's user data points to this structure, i.e.
    s.uid.get_data() is a pointer to s. Notice that the rate list
    is generated by the type checker; until then the rate
    expressions are part of the invariant.
*/
struct location_t : stringify_t<location_t>
{
    symbol_t uid;           /**< The symbol of the location */
    expression_t name;      /**< TODO: the location name with its position */
    expression_t invariant; /**< The invariant */
    expression_t exp_rate;  /**< the exponential rate controlling the speed of leaving the location */
    expression_t cost_rate; /**< cost rate/derivative expression */
    int32_t nr;             /**< Location number in a template */
    std::ostream& print(std::ostream&) const;
};

/** Information about a branchpoint.
    Branchpoints may be used in construction of edges with the
    same source, guard and synchronisation channel.
    They are not present after compilation of a model.
 */
struct branchpoint_t
{
    symbol_t uid;
    int32_t bpNr;
};

/** Information about an edge.  Edges have a source (src) and a
    destination (dst), which may be locations or branchpoints.
    The unused of these pointers should be set to nullptr.
    The guard, synchronisation and assignment are stored as
    expressions.
*/
struct edge_t : stringify_t<edge_t>
{
    int nr;       /**< Placement in input file */
    bool control; /**< Controllable (true/false) */
    std::string actname;
    location_t* src{nullptr};          /**< Pointer to source location */
    branchpoint_t* srcb{nullptr};      /**< Pointer to source branchpoint */
    location_t* dst{nullptr};          /**< Pointer to destination location */
    branchpoint_t* dstb{nullptr};      /**< Pointer to destination branchpoint */
    frame_t select;                    /**< Frame for non-deterministic select */
    expression_t guard;                /**< The guard */
    expression_t assign;               /**< The assignment */
    expression_t sync;                 /**< The synchronisation */
    expression_t prob;                 /**< Probability for probabilistic edges. */
    std::vector<int32_t> selectValues; /**<The select values, if any */
    std::ostream& print(std::ostream&) const;
};

class BlockStatement;  // Forward declaration

/** Information about a function. The symbol's user data points to
    this structure, i.e. f.uid.get_data() is a pointer to f.
*/
struct function_t : stringify_t<function_t>
{
    symbol_t uid;                                  /**< The symbol of the function. */
    std::set<symbol_t> changes{};                  /**< Variables changed by this function. */
    std::set<symbol_t> depends{};                  /**< Variables the function depends on. */
    std::list<variable_t> variables{};             /**< Local variables. List is used for stable pointers. */
    std::unique_ptr<BlockStatement> body{nullptr}; /**< Pointer to the block. */
    function_t() = default;
    std::ostream& print(std::ostream& os) const; /**< textual representation, used to write the XML file */
    position_t body_position;
};

struct progress_t
{
    expression_t guard;
    expression_t measure;
    progress_t(expression_t guard, expression_t measure): guard{std::move(guard)}, measure{std::move(measure)} {}
};

struct iodecl_t
{
    std::string instanceName;
    std::vector<expression_t> param;
    std::list<expression_t> inputs, outputs, csp;
};

/**
 * Gantt map bool expr -> int expr that
 * can be expanded.
 */
struct ganttmap_t
{
    frame_t parameters;
    expression_t predicate, mapping;
};

/**
 * Gantt chart entry.
 */
struct gantt_t
{
    std::string name;   /**< The name */
    frame_t parameters; /**< The select parameters */
    std::list<ganttmap_t> mapping;
    explicit gantt_t(std::string name): name{std::move(name)} {}
};

/**
 * Structure holding declarations of various types. Used by
 * templates and block statements.
 */
struct template_t;
struct declarations_t : stringify_t<declarations_t>
{
    frame_t frame;
    std::list<variable_t> variables; /**< Variables. List is used for stable pointers. */
    std::list<function_t> functions; /**< Functions */
    std::list<progress_t> progress;  /**< Progress measures */
    std::list<iodecl_t> iodecl;
    std::list<gantt_t> ganttChart;

    /** Add function declaration. */
    bool add_function(type_t type, std::string name, position_t, function_t*&);
    /** The following methods are used to write the declarations in an XML file */
    std::string str(bool global) const;
    std::ostream& print(std::ostream&, bool global = false) const;
    std::ostream& print_constants(std::ostream&) const;
    std::ostream& print_typedefs(std::ostream&) const;
    std::ostream& print_variables(std::ostream&, bool global) const;
    std::ostream& print_functions(std::ostream&) const;
};

struct instance_line_t;  // to be defined later

/** Common members among LSC elements */
struct LSC_element_t
{
    int nr{-1}; /**< Placement in input file */
    int location{-1};
    bool is_in_prechart{false};
    int get_nr() const { return nr; }
    bool empty() const { return nr == -1; }
};

/** Information about a message. Messages have a source (src) and a
 * destination (dst) instance lines. The label is
 * stored as an expression.
 */
struct message_t : LSC_element_t
{
    instance_line_t* src{nullptr}; /**< Pointer to source instance line */
    instance_line_t* dst{nullptr}; /**< Pointer to destination instance line */
    expression_t label;            /**< The label */
};
/** Information about a condition. Conditions have an anchor instance lines.
 * The label is stored as an expression.
 */
struct condition_t : LSC_element_t
{
    std::vector<instance_line_t*> anchors{}; /**< Pointer to anchor instance lines */
    expression_t label;                      /**< The label */
    bool isHot{false};
};

/** Information about an update. Update have an anchor instance line.
 * The label is stored as an expression.
 */
struct update_t : LSC_element_t
{
    instance_line_t* anchor{nullptr}; /**< Pointer to anchor instance line */
    expression_t label;               /**< The label */
};

struct simregion_t : stringify_t<simregion_t>
{
    int nr{};
    message_t* message;     /** May be empty */
    condition_t* condition; /** May be empty */
    update_t* update;       /** May be empty */

    int get_loc() const;
    bool is_in_prechart() const;

    simregion_t()
    {
        message = new message_t();
        condition = new condition_t();
        update = new update_t();
    }

    ~simregion_t() noexcept
    {
        delete message;
        delete condition;
        delete update;
    }

    std::ostream& print(std::ostream&) const;
    bool has_message() const { return message != nullptr && !message->empty(); }
    bool has_condition() const { return condition != nullptr && !condition->empty(); }
    bool has_update() const { return update != nullptr && !update->empty(); }
    void set_message(std::deque<message_t>& messages, int nr);
    void set_condition(std::deque<condition_t>& conditions, int nr);
    void set_update(std::deque<update_t>& updates, int nr);
};

struct compare_simregion
{
    bool operator()(const simregion_t& x, const simregion_t& y) const { return (x.get_loc() < y.get_loc()); }
};

struct cut_t : stringify_t<cut_t>
{
    int nr;
    std::vector<simregion_t> simregions{};  // unordered
    explicit cut_t(int number): nr{number} {};
    void add(const simregion_t& s) { simregions.push_back(s); };
    void erase(const simregion_t& s);
    bool contains(const simregion_t& s) const;

    /**
     * returns true if the cut is in the prechart,
     * given one of the following simregions.
     * if one of the following simregions is not in the prechart,
     * then all following simregions aren't in the prechart (because of the
     * construction of the partial order),
     * and the cut is not in the prechart (but may contain only simregions
     * that are in the prechart, if it is the limit between the prechart
     * and the mainchart)
     */
    bool is_in_prechart(const simregion_t& fSimregion) const;
    bool is_in_prechart() const;

    bool equals(const cut_t& y) const;
    std::ostream& print(std::ostream&) const;
};

/**
 * Partial instance of a template. Every template is also a
 * partial instance of itself and therefore template_t is derived
 * from instance_t. A complete instance is just a partial instance
 * without any parameters.
 *
 * Even though it is possible to make partial instances of partial
 * instances, they are not represented hierarchically: All
 * parameters and arguments are merged into this one
 * struct. Therefore \a parameters contains both bound and unbound
 * symbols: Unbound symbols are parameters of this instance. Bound
 * symbols are inherited from another instance. Symbols in \a
 * parameters are ordered such that unbound symbols are listed
 * first, i.e., uid.get_type().size() == parameters.get_size().
 *
 * \a mapping binds parameters to expressions.
 *
 * \a arguments is the number of arguments given by the partial
 * instance. The first \a arguments bound symbols of \a parameters
 * are the corresponding parameters. For templates, \a arguments
 * is obviously 0.
 *
 * Restricted variables are those that are used either directly or
 * indirectly in the definition of array sizes. Any restricted
 * parameters have restriction on the kind of arguments they
 * accept (they must not depend on any free process parameters).
 *
 * If i is an instance, then i.uid.get_data() == i.
 */
struct instance_t
{
    symbol_t uid{};                           /**< The name */
    frame_t parameters{};                     /**< The parameters */
    std::map<symbol_t, expression_t> mapping; /**< The parameter to argument mapping */
    size_t arguments{0};
    size_t unbound{0}; /**< The number of unbound parameters */
    struct template_t* templ{nullptr};
    std::set<symbol_t> restricted; /**< Restricted variables */

    std::ostream& print_mapping(std::ostream&) const;
    std::ostream& print_parameters(std::ostream&) const;
    std::ostream& print_arguments(std::ostream&) const;
    std::string mapping_str() const;
    std::string parameters_str() const;
    std::string arguments_str() const;
};

/** Information about an instance line.
 */
struct instance_line_t : public instance_t
{
    int32_t instance_nr; /**< InstanceLine number in template */
    std::vector<simregion_t> getSimregions(const std::vector<simregion_t>& simregions);
    void add_parameters(instance_t& inst, frame_t params, const std::vector<expression_t>& arguments);
};

struct template_t : public instance_t, declarations_t
{
    symbol_t init{};                        /**< The initial location */
    frame_t template_set{};                 /**< Template set decls */
    std::deque<location_t> locations;       /**< Locations */
    std::deque<branchpoint_t> branchpoints; /**< Branchpoints */
    std::deque<edge_t> edges;               /**< Edges */
    std::vector<expression_t> dynamic_evals;
    bool is_TA{true};
    bool is_instantiated{false}; /**< Is the template used in the system*/

    int add_dynamic_eval(expression_t t)
    {
        dynamic_evals.push_back(t);
        return dynamic_evals.size() - 1;
    }

    std::vector<expression_t>& get_dynamic_eval() { return dynamic_evals; }

    /** Add another location to template. */
    location_t& add_location(const std::string& name, expression_t inv, expression_t er, position_t pos);

    /** Add another branchpoint to template. */
    branchpoint_t& add_branchpoint(const std::string&, position_t);

    /** Add edge to template. */
    edge_t& add_edge(symbol_t src, symbol_t dst, bool type, std::string actname);

    std::deque<instance_line_t> instances; /**< Instance Lines */
    std::deque<message_t> messages;        /**< Messages */
    std::deque<update_t> updates;          /**< Updates */
    std::deque<condition_t> conditions;    /**< Conditions */
    std::string type;
    std::string mode;
    bool has_prechart{false};
    bool dynamic{false};
    int dyn_index{0};
    bool is_defined{false};

    /** Add another instance line to template. */
    instance_line_t& add_instance_line();

    /** Add message to template. */
    message_t& add_message(symbol_t src, symbol_t dst, int loc, bool pch);

    /** Add condition to template. */
    condition_t& add_condition(std::vector<symbol_t> anchors, int loc, bool pch, bool isHot);

    /** Add update to template. */
    update_t& add_update(symbol_t anchor, int loc, bool pch);

    bool is_invariant() const;  // type of the LSC

    /* gets the simregions from the LSC scenario */
    const std::vector<simregion_t> get_simregions();

    /* returns the condition on the given instance, at y location */
    bool get_condition(instance_line_t& instance, int y, condition_t*& simCondition);

    /* returns the update on the given instance at y location */
    bool get_update(instance_line_t& instance, int y, update_t*& simUpdate);

    /* returns the first update on one of the given instances, at y location */
    bool get_update(std::vector<instance_line_t*>& instances, int y, update_t*& simUpdate);
};

/**
 * Channel priority information. Expressions must evaluate to
 * a channel or an array of channels.
 */
struct chan_priority_t : stringify_t<chan_priority_t>
{
    using entry = std::pair<char, expression_t>;
    using tail_t = std::list<entry>;

    expression_t head;  //!< First expression in priority declaration
    tail_t tail;        //!< Pairs: separator and channel expressions

    std::ostream& print(std::ostream&) const;
};

enum class expectation_type { Symbolic, Probability, NumericValue, _ErrorValue };

enum class query_status_t { True, False, MaybeTrue, MaybeFalse, Unknown };

struct option_t
{
    std::string name;
    std::string value;
    option_t(std::string name, std::string value): name{std::move(name)}, value{std::move(value)} {}
};

enum class resource_type { Time, Memory };

struct resource_t
{
    std::string name;
    std::string value;
    std::optional<std::string> unit;
};

using resources_t = std::vector<resource_t>;
using options_t = std::vector<option_t>;

struct results_t
{
    options_t options;  // options used for results
    std::string message;
    std::string value;  // REVISIT maybe should be variant or similar with actual value?
};
struct expectation_t
{
    expectation_type value_type;
    query_status_t status;
    std::string value;
    resources_t resources;
};

struct query_t
{
    std::string formula;
    std::string comment;
    options_t options;
    expectation_t expectation;
    // std::vector<results_t> results;
    std::string location;
};
using queries_t = std::vector<query_t>;

class Document;

class DocumentVisitor
{
public:
    virtual ~DocumentVisitor() noexcept = default;
    virtual void visitDocBefore(Document&) {}
    virtual void visitDocAfter(Document&) {}
    virtual void visitVariable(variable_t&) {}
    virtual bool visitTemplateBefore(template_t&) { return true; }
    virtual void visitTemplateAfter(template_t&) {}
    virtual void visitLocation(location_t&) {}
    virtual void visitEdge(edge_t&) {}
    virtual void visitInstance(instance_t&) {}
    virtual void visitProcess(instance_t&) {}
    virtual void visitFunction(function_t&) {}
    virtual void visitTypeDef(symbol_t) {}
    virtual void visitIODecl(iodecl_t&) {}
    virtual void visitProgressMeasure(progress_t&) {}
    virtual void visitGanttChart(gantt_t&) {}
    virtual void visitInstanceLine(instance_line_t&) {}
    virtual void visitMessage(message_t&) {}
    virtual void visitCondition(condition_t&) {}
    virtual void visitUpdate(update_t&) {}
};

class Document
{
public:
    Document();
    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;

    /** Returns the global declarations of the document. */
    declarations_t& get_globals() { return global; }

    /** Returns the templates of the document. */
    std::list<template_t>& get_templates() { return templates; }
    const template_t* find_template(const std::string& name) const;
    std::vector<template_t*>& get_dynamic_templates();
    template_t* find_dynamic_template(const std::string& name);

    /** Returns the processes of the document. */
    std::list<instance_t>& get_processes() { return processes; }

    /** Returns the instances of the document. */
    const std::list<instance_t>& get_instances() const { return instances; }

    options_t& get_options();
    void set_options(const options_t& options);

    /** Returns the queries enclosed in the model. */
    const queries_t& get_queries() const { return queries; }

    void add_position(uint32_t position, uint32_t offset, uint32_t line, std::shared_ptr<std::string> path);
    const position_index_t::line_t& find_position(uint32_t position) const;
    const position_index_t::line_t& find_first_position(uint32_t position) const;

    variable_t* add_variable_to_function(function_t*, frame_t, type_t, const std::string&, expression_t initital,
                                         position_t);
    variable_t* add_variable(declarations_t*, type_t type, const std::string&, expression_t initial, position_t);
    void add_progress_measure(declarations_t*, expression_t guard, expression_t measure);

    template_t& add_template(const std::string& name, frame_t params, position_t, bool isTA = true,
                             const std::string& type = "", const std::string& mode = "");
    template_t& add_dynamic_template(const std::string& name, frame_t params, position_t pos);

    instance_t& add_instance(const std::string& name, instance_t& instance, frame_t params,
                             const std::vector<expression_t>& arguments, position_t);

    instance_t& add_LSC_instance(const std::string& name, instance_t& instance, frame_t params,
                                 const std::vector<expression_t>& arguments, position_t);
    void remove_process(instance_t& instance);  // LSC

    void copy_variables_from_to(const template_t* from, template_t* to) const;
    void copy_functions_from_to(const template_t* from, template_t* to) const;

    std::string obsTA;  // name of the observer TA instance

    void add_process(instance_t& instance, position_t);
    void add_gantt(declarations_t*, gantt_t);  // copies gantt_t and moves it
    void accept(DocumentVisitor&);

    void set_before_update(expression_t);
    expression_t get_before_update();
    void set_after_update(expression_t);
    expression_t get_after_update();

    void add_query(query_t query);  // creates a copy and moves it
    bool queriesEmpty() const;

    /* The default priority for channels is also used for 'tau
     * transitions' (i.e. non-synchronizing transitions).
     */
    void begin_chan_priority(expression_t chan);
    void add_chan_priority(char separator, expression_t chan);
    const std::list<chan_priority_t>& get_chan_priorities() const { return chan_priorities; }
    std::list<chan_priority_t>& get_chan_priorities() { return chan_priorities; }

    /** Sets process priority for process \a name. */
    void set_proc_priority(const std::string& name, int priority);

    /** Returns process priority for process \a name. */
    int get_proc_priority(const char* name) const;

    /** Returns true if document has some priority declaration. */
    bool has_priority_declaration() const { return hasPriorities; }

    /** Returns true if document has some strict invariant. */
    bool has_strict_invariants() const { return hasStrictInv; }

    /** Record that the document has some strict invariant. */
    void record_strict_invariant() { hasStrictInv = true; }

    /** Returns true if the document stops any clock. */
    bool has_stop_watch() const { return stopsClock; }

    /** Record that the document stops a clock. */
    void record_stop_watch() { stopsClock = true; }

    /** Returns true if the document has guards on controllable edges with strict lower bounds. */
    bool has_strict_lower_bound_on_controllable_edges() const { return hasStrictLowControlledGuards; }

    /** Record that the document has guards on controllable edges with strict lower bounds. */
    void record_strict_lower_bound_on_controllable_edges() { hasStrictLowControlledGuards = true; }

    void clock_guard_recv_broadcast() { hasGuardOnRecvBroadcast = true; }
    bool has_clock_guard_recv_broadcast() const { return hasGuardOnRecvBroadcast; }
    void set_sync_used(int s) { syncUsed = s; }
    int get_sync_used() const { return syncUsed; }

    void set_urgent_transition() { hasUrgentTrans = true; }
    bool has_urgent_transition() const { return hasUrgentTrans; }
    bool has_dynamic_templates() const { return !dyn_templates.empty(); }

    const std::vector<std::string>& get_strings() const { return strings; }
    void add_string(const std::string& string) { strings.push_back(string); }
    size_t add_string_if_new(const std::string& string)
    {
        auto it = std::find(std::begin(strings), std::end(strings), string);
        if (it == std::end(strings)) {
            strings.push_back(string);
            return strings.size() - 1;
        } else {
            return std::distance(std::begin(strings), it);
        }
    }

protected:
    bool hasUrgentTrans{false};
    bool hasPriorities{false};
    bool hasStrictInv{false};
    bool stopsClock{false};
    bool hasStrictLowControlledGuards{false};
    bool hasGuardOnRecvBroadcast{false};
    int defaultChanPriority{0};
    std::list<chan_priority_t> chan_priorities;
    std::map<std::string, int> proc_priority;
    int syncUsed{0};  // see typechecker

    // The list of templates.
    std::list<template_t> templates;
    // List of dynamic templates
    std::list<template_t> dyn_templates;
    std::vector<template_t*> dyn_templates_vec;

    // The list of template instances.
    std::list<instance_t> instances;

    std::list<instance_t> lsc_instances;
    bool modified{false};

    // List of processes.
    std::list<instance_t> processes;

    // Global declarations
    declarations_t global;

    expression_t before_update;
    expression_t after_update;
    options_t model_options;
    queries_t queries;

    variable_t* add_variable(std::list<variable_t>& variables, frame_t frame, type_t type, const std::string&,
                             position_t);

    std::string location;
    std::vector<library_t> libraries;
    std::vector<std::string> strings;
    SupportedMethods supported_methods{};

public:
    void add(library_t&& lib);
    /** Returns the last successfully loaded library, or throws std::runtime_error. */
    library_t& last_library();
    void add_error(position_t, std::string msg, std::string ctx = "");
    void add_warning(position_t, const std::string& msg, const std::string& ctx = "");
    bool has_errors() const { return !errors.empty(); }
    bool has_warnings() const { return !warnings.empty(); }
    const std::vector<error_t>& get_errors() const { return errors; }
    const std::vector<error_t>& get_warnings() const { return warnings; }
    void clear_errors() const { errors.clear(); }
    void clear_warnings() const { warnings.clear(); }
    bool is_modified() const { return modified; }
    void set_modified(bool mod) { modified = mod; }
    iodecl_t* add_io_decl();
    void set_supported_methods(const SupportedMethods& supportedMethods);
    const SupportedMethods& get_supported_methods() const { return supported_methods; }
    const position_index_t& get_positions() const { return positions; }

private:
    // TODO: move errors & warnings to ParserBuilder to get rid of mutable
    mutable std::vector<error_t> errors;
    mutable std::vector<error_t> warnings;
    position_index_t positions;
};
}  // namespace UTAP

#endif /* UTAP_INTERMEDIATE_HH */
