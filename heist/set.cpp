/**
 * Heist immutable/functional data structure library
 * Copyright (C) 2016-2017 by Stephen Blackheath
 * Released under a BSD3 licence
 */
#include <heist/set.h>
#include <stdexcept>


namespace heist {
    namespace impl {

        std::tuple<const Ptr*, const Ptr*, const Ptr*> insertItem(
            std::function<int(const Ptr&, const Ptr&)> compare, const Ptr& x, const Ptr& a, const Ptr& b) {
            if (compare(x, a) < 0) return std::tuple<const Ptr*, const Ptr*, const Ptr*>(&x, &a, &b); else
            if (compare(x, b) < 0) return std::tuple<const Ptr*, const Ptr*, const Ptr*>(&a, &x, &b); else
                                   return std::tuple<const Ptr*, const Ptr*, const Ptr*>(&a, &b, &x);
        }

        void nullDeleter(void*) {}

        const Ptr& iterator::get() const
        {
            const Position& top = stack.head();
            int ix = top.ix;
            if (const Leaf1* l1 = boost::get<Leaf1>(&top.node.n)) return l1->a; else                   // 0      / 1
            if (const Leaf2* l2 = boost::get<Leaf2>(&top.node.n)) return ix == 0 ? l2->a : l2->b; else // 0 or 1 / 2
            if (const Node2* n2 = boost::get<Node2>(&top.node.n)) return n2->a; else {                 // 1      / 3
                const Node3& n3 = boost::get<Node3>(top.node.n);                                       // 1 or 3 / 5
                return ix == 1 ? n3.a : n3.b;
            }
        }
    
        struct MoveVisitor : public boost::static_visitor<heist::list<Position>>
        {
            MoveVisitor(const heist::list<Position>& stack, int dir, int ix)
                : stack(stack), dir(dir), ix(ix) {}
            heist::list<Position> stack;
            int dir;
            int ix;
            heist::list<Position> operator()(const Leaf1& l1) const {
                return Position(Node(l1), 0) %= stack;
            }
            heist::list<Position> operator()(const Leaf2& l2) const {
                int ix = this->ix < 0 ? (dir < 0 ? 1 : 0) : this->ix;
                return Position(Node(l2), ix) %= stack;
            }
            heist::list<Position> operator()(const Node2& n2) const {
                int ix = this->ix < 0 ? (dir < 0 ? 2 : 0) : this->ix;
                auto push = [&] () {
                    return Position(Node(n2), ix) %= this->stack;
                };
                switch (ix) {
                    case 0:
                        return boost::apply_visitor(MoveVisitor(push(), dir, -1), ((const Node*)n2.p.value)->n);
                    case 2:
                        return boost::apply_visitor(MoveVisitor(push(), dir, -1), ((const Node*)n2.q.value)->n);
                    default:
                        return Position(Node(n2), ix) %= stack;
                }
            }
            heist::list<Position> operator()(const Node3& n3) const {
                int ix = this->ix < 0 ? (dir < 0 ? 4 : 0) : this->ix;
                auto push = [&] () {
                    return Position(Node(n3), ix) %= this->stack;
                };
                switch (ix) {
                    case 0:
                        return boost::apply_visitor(MoveVisitor(push(), dir, -1), ((const Node*)n3.p.value)->n);
                    case 2:
                        return boost::apply_visitor(MoveVisitor(push(), dir, -1), ((const Node*)n3.q.value)->n);
                    case 4:
                        return boost::apply_visitor(MoveVisitor(push(), dir, -1), ((const Node*)n3.r.value)->n);
                    default:
                        return Position(Node(n3), ix) %= stack;
                }
            }
        };
    
        iterator Node::begin() const
        {
            return boost::apply_visitor(MoveVisitor(heist::list<Position>(), 1, -1), n);
        }
    
        iterator Node::end() const
        {
            return boost::apply_visitor(MoveVisitor(heist::list<Position>(), -1, -1), n);
        }
    
        boost::optional<iterator> iterator::move(int dir) const
        {
            const Position& top = stack.head();
            int nextIx = top.ix+dir;
            if (nextIx >= 0 && nextIx < top.node.getNoOfIndices())
                return boost::make_optional(iterator(
                    boost::apply_visitor(MoveVisitor(stack.tail(), dir, nextIx), top.node.n)
                ));
            else
            if (stack.tail()) {
                return iterator(stack.tail()).move(dir);
            }
            else
                return boost::optional<iterator>();
        }
    
        struct LowerBoundVisitor : public boost::static_visitor<boost::optional<heist::list<Position>>>
        {
            LowerBoundVisitor(const Comparator& compare, const heist::list<Position>& stack, const Ptr& x)
                : compare(compare), stack(stack), x(x) {}
            Comparator compare;
            heist::list<Position> stack;
            const Ptr& x;
            boost::optional<heist::list<Position>> operator()(const Leaf1& l1) const {
                auto push = [&] (int ix) -> heist::list<Position> {
                    return Position(Node(l1), ix) %= this->stack;
                };
                if (compare(l1.a, x) >= 0)
                    return boost::make_optional(push(0));
                else
                    return boost::optional<heist::list<Position>>();
            }
            boost::optional<heist::list<Position>> operator()(const Leaf2& l2) const {
                auto push = [&] (int ix) -> heist::list<Position> {
                    return Position(Node(l2), ix) %= this->stack;
                };
                if (compare(l2.a, x) >= 0)
                    return boost::make_optional(push(0));
                else
                if (compare(l2.b, x) >= 0)
                    return boost::make_optional(push(1));
                else
                    return boost::optional<heist::list<Position>>();
            }
            boost::optional<heist::list<Position>> operator()(const Node2& n2) const {
                auto push = [&] (int ix) -> heist::list<Position> {
                    return Position(Node(n2), ix) %= this->stack;
                };
                if (compare(n2.a, x) >= 0) {
                    auto child = boost::apply_visitor(LowerBoundVisitor(compare, push(0), x), ((const Node*)n2.p.value)->n);
                    if (child)
                        return child;
                    else
                        return boost::make_optional(push(1));
                }
                else
                    return boost::apply_visitor(LowerBoundVisitor(compare, push(2), x), ((const Node*)n2.q.value)->n);
            }
            boost::optional<heist::list<Position>> operator()(const Node3& n3) const {
                auto push = [&] (int ix) -> heist::list<Position> {
                    return Position(Node(n3), ix) %= this->stack;
                };
                if (compare(n3.a, x) >= 0) {
                    auto child = boost::apply_visitor(LowerBoundVisitor(compare, push(0), x), ((const Node*)n3.p.value)->n);
                    if (child)
                        return child;
                    else
                        return boost::make_optional(push(1));
                }
                else
                if (compare(n3.b, x) >= 0) {
                    auto child = boost::apply_visitor(LowerBoundVisitor(compare, push(2), x), ((const Node*)n3.q.value)->n);
                    if (child)
                        return child;
                    else
                        return boost::make_optional(push(3));
                }
                else
                    return boost::apply_visitor(LowerBoundVisitor(compare, push(4), x), ((const Node*)n3.r.value)->n);
            }
        };
    
        boost::optional<iterator> Node::lower_bound(const Comparator& compare, const Ptr& a) const
        {
            auto o = boost::apply_visitor(LowerBoundVisitor(compare, heist::list<Position>(), a), n);
            return o ? boost::optional<iterator>(iterator(o.get()))
                     : boost::optional<iterator>();
        }
    
        boost::optional<iterator> Node::find(const Comparator& compare, const Ptr& a) const
        {
            auto oit = lower_bound(compare, a);
            // If lower_bound found something but it didn't equal what we asked for...
            // then return "not found"
            if (oit && compare(oit.get().get(), a) != 0)
                return boost::optional<iterator>();
            else
                return oit;
        }
    
        bool isTerminal(const Node& node)
        {
            return boost::get<Leaf1>(&node.n) != NULL ||
                   boost::get<Leaf2>(&node.n) != NULL;
        }
    
        Node iterator::unwind() const
        {
            if (!stack.tail())
                return stack.head().node;
            else {
                const Position& top = stack.head();
                const Position& parent = stack.tail().head();
                if (const Node2* n2 = boost::get<Node2>(&parent.node.n)) {
                    Ptr pNode = Ptr::create<Node>(top.node);
                    switch (parent.ix) {
                        case 0: return iterator(
                                Position(Node(Node2(pNode, n2->a, n2->q)), 0)
                                %= stack.tail().tail()
                            ).unwind();
                        case 2: return iterator(
                                Position(Node(Node2(n2->p, n2->a, pNode)), 0)
                                %= stack.tail().tail()
                            ).unwind();
                        default: throw std::runtime_error("iterator::unwind() impossible");
                    }
                }
                else {
                    const Node3& n3 = boost::get<Node3>(parent.node.n);
                    Ptr pNode = Ptr::create<Node>(top.node);
                    switch (parent.ix) {
                        case 0: return iterator(
                                Position(Node(Node3(pNode, n3.a, n3.q, n3.b, n3.r)), 0)
                                %= stack.tail().tail()
                            ).unwind();
                        case 2: return iterator(
                                Position(Node(Node3(n3.p, n3.a, pNode, n3.b, n3.r)), 0)
                                %= stack.tail().tail()
                            ).unwind();
                        case 4: return iterator(
                                Position(Node(Node3(n3.p, n3.a, n3.q, n3.b, pNode)), 0)
                                %= stack.tail().tail()
                            ).unwind();
                        default: throw std::runtime_error("iterator::unwind() impossible");
                    }
                }
            }
        }

        bool is_two_node(const Node& node)
        {
            return boost::get<Leaf1>(&node.n) != NULL ||
                   boost::get<Node2>(&node.n) != NULL;
        }
    
        boost::optional<Node> bubble(
            std::function<boost::optional<Node>()> mk0,
            std::function<Node(const Ptr&, const Node&)> mk3Left,
            std::function<Node(const Node&, const Ptr&)> mk3Right,
            std::function<std::tuple<Node, const Ptr*, Node>(const Ptr&, const Node&)> splitLeft,
            std::function<std::tuple<Node, const Ptr*, Node>(const Node&, const Ptr&)> splitRight,
            const heist::list<Position>& stack
        )
        {
            if (!stack)
                return mk0();
            const Position& top = stack.head();
            if (const Node2* parent1 = boost::get<Node2>(&top.node.n)) {
                const Node& sibling = *((const Node*)(top.ix == 0 ? parent1->q.value : parent1->p.value));
                if (is_two_node(sibling)) {
                    Node newNode(
                        top.ix == 0 ? mk3Left(parent1->a, sibling)
                                    : mk3Right(sibling, parent1->a)
                    );
                    // case 1: 2-node as parent, 2-node as sibling
                    return bubble(
                        [newNode] () {
                            return boost::make_optional(newNode);
                        },
                        // mk3Left
                        [newNode] (const Ptr& l, const Node& r) {
                            const Node2& n2 = boost::get<Node2>(r.n);
                            return Node(Node3(
                                mkPtr<Node>(new Node(newNode)), l, n2.p, n2.a, n2.q
                            ));
                        },
                        // mk3Right
                        [newNode] (const Node& l, const Ptr& r) {
                            const Node2& n2 = boost::get<Node2>(l.n);
                            return Node(Node3(
                                n2.p, n2.a, n2.q, r, mkPtr<Node>(new Node(newNode))
                            ));
                        },
                        // splitLeft
                        [newNode] (const Ptr& l, const Node& r) -> std::tuple<Node, const Ptr*, Node> {
                            const Node3& n3 = boost::get<Node3>(r.n);
                            return std::make_tuple(
                                Node(Node2(mkPtr<Node>(new Node(newNode)), l, n3.p)),
                                &n3.a,
                                Node(Node2(n3.q, n3.b, n3.r))
                            );
                        },
                        // splitRight
                        [newNode] (const Node& l, const Ptr& r) -> std::tuple<Node, const Ptr*, Node> {
                            const Node3& n3 = boost::get<Node3>(l.n);
                            return std::make_tuple(
                                Node(Node2(n3.p, n3.a, n3.q)),
                                &n3.b,
                                Node(Node2(n3.r, r, mkPtr<Node>(new Node(newNode))))
                            );
                        },
                        stack.tail()
                    );
                }
                else {
                    // case 2: 2-node as parent, 3-node as sibling
                    if (top.ix == 0) {
                        auto tpl = splitLeft(parent1->a, sibling);
                        return boost::make_optional(
                            iterator(
                                Position(Node(Node2(
                                    mkPtr<Node>(new Node(std::get<0>(tpl))),
                                    *std::get<1>(tpl),
                                    mkPtr<Node>(new Node(std::get<2>(tpl)))
                                )), 0)
                                %= stack.tail()
                            ).unwind()
                        );
                    }
                    else {
                        auto tpl = splitRight(sibling, parent1->a);
                        return boost::make_optional(
                            iterator(
                                Position(Node(Node2(
                                    mkPtr<Node>(new Node(std::get<0>(tpl))),
                                    *std::get<1>(tpl),
                                    mkPtr<Node>(new Node(std::get<2>(tpl)))
                                )), 0)
                                %= stack.tail()
                            ).unwind()
                        );
                    }
                }
            }
            else {
                const Node3& parent = boost::get<Node3>(top.node.n);
                switch (top.ix) {
                    case 0:
                        if (is_two_node(*(const Node*)parent.q.value)) {  // case 3(a)
                            return boost::make_optional(
                                iterator(
                                    Position(
                                        Node(Node2(
                                            mkPtr<Node>(new Node(mk3Left(parent.a, *(const Node*)parent.q.value))),
                                            parent.b,
                                            parent.r
                                        )), 0
                                    )
                                    %= stack.tail()
                                ).unwind()
                            );
                        }
                        else {  // case 4(a)
                            auto tpl = splitLeft(parent.a, *(const Node*)parent.q.value);
                            return boost::make_optional(
                                iterator(
                                    Position(
                                        Node(Node3(
                                            mkPtr<Node>(new Node(std::get<0>(tpl))),
                                            *std::get<1>(tpl),
                                            mkPtr<Node>(new Node(std::get<2>(tpl))),
                                            parent.b,
                                            parent.r
                                        )), 0
                                    )
                                    %= stack.tail()
                                ).unwind()
                            );
                        }
                    case 2:
                        if (is_two_node(*(const Node*)parent.p.value)) {  // case 3(a)
                            return boost::make_optional(
                                iterator(
                                    Position(
                                        Node(Node2(
                                            mkPtr<Node>(new Node(mk3Right(*(const Node*)parent.p.value, parent.a))),
                                            parent.b,
                                            parent.r
                                        )), 0
                                    )
                                    %= stack.tail()
                                ).unwind()
                            );
                        }
                        else {  // case 4(a)
                            auto tpl = splitRight(*(const Node*)parent.p.value, parent.a);
                            return boost::make_optional(
                                iterator(
                                    Position(
                                        Node(Node3(
                                            mkPtr<Node>(new Node(std::get<0>(tpl))),
                                            *std::get<1>(tpl),
                                            mkPtr<Node>(new Node(std::get<2>(tpl))),
                                            parent.b,
                                            parent.r
                                        )), 0
                                    )
                                    %= stack.tail()
                                ).unwind()
                            );
                        }
                    case 4:
                        if (is_two_node(*(const Node*)parent.q.value)) {  // case 3(b)
                            return boost::make_optional(
                                iterator(
                                    Position(
                                        Node(Node2(
                                            parent.p,
                                            parent.a,
                                            mkPtr<Node>(new Node(mk3Right(*(const Node*)parent.q.value, parent.b)))
                                        )), 0
                                    )
                                    %= stack.tail()
                                ).unwind()
                            );
                        }
                        else {  // case 4(b)
                            auto tpl = splitRight(*(const Node*)parent.q.value, parent.b);
                            return boost::make_optional(
                                iterator(
                                    Position(
                                        Node(Node3(
                                            parent.p,
                                            parent.a,
                                            mkPtr<Node>(new Node(std::get<0>(tpl))),
                                            *std::get<1>(tpl),
                                            mkPtr<Node>(new Node(std::get<2>(tpl)))
                                        )), 0
                                    )
                                    %= stack.tail()
                                ).unwind()
                            );
                        }
                    default:
                        throw std::runtime_error("bubble impossible");
                }  // switch
            }
        }
    
        boost::optional<Node> iterator::remove() const
        {
            const Position& top = stack.head();
            // Cases where we're deleting a non-terminal node and we do this:
            // 1. Grab the value of the successor or predecessor and replace the value
            //    to delete with that.
            // 2. Delete the successor/predecessor.
            if (const Node3* n3 = boost::get<Node3>(&top.node.n)) {
                if (top.ix == 1)
                    return iterator(
                        Position(Node(Node3(n3->p, this->next().get().get(), n3->q, n3->b, n3->r)), top.ix)
                        %= this->stack.tail()
                    ).next().get().remove();
                else
                    return iterator(
                        Position(Node(Node3(n3->p, n3->a, n3->q, this->prev().get().get(), n3->r)), top.ix)
                        %= this->stack.tail()
                    ).prev().get().remove();
            }
            else
            if (const Node2* n2 = boost::get<Node2>(&top.node.n)) {
                return iterator(
                    Position(Node(Node2(n2->p, this->next().get().get(), n2->q)), top.ix)
                    %= this->stack.tail()
                ).next().get().remove();
            }
            else
            if (const Leaf2* l2 = boost::get<Leaf2>(&top.node.n)) {
                return boost::make_optional(
                    iterator(
                        Position(Node(Leaf1(top.ix == 0 ? l2->b : l2->a)), 0) %=
                        this->stack.tail()
                    ).unwind()
                );
            }
            else {
                return bubble(
                    [] () {
                        return boost::optional<Node>();
                    },
                    // mk3Left
                    [] (const Ptr& l, const Node& r) {
                        return Node(Leaf2(l, boost::get<Leaf1>(r.n).a));
                    },
                    // mk3Right
                    [] (const Node& l, const Ptr& r) {
                        return Node(Leaf2(boost::get<Leaf1>(l.n).a, r));
                    },
                    // splitLeft
                    [] (const Ptr& l, const Node& r) -> std::tuple<Node, const Ptr*, Node> {
                        const Leaf2& l2 = boost::get<Leaf2>(r.n);
                        return std::make_tuple(
                            Node(Leaf1(l)),
                            &l2.a,
                            Node(Leaf1(l2.b))
                        );
                    },
                    // splitRight
                    [] (const Node& l, const Ptr& r) -> std::tuple<Node, const Ptr*, Node> {
                        const Leaf2& l2 = boost::get<Leaf2>(l.n);
                        return std::make_tuple(
                            Node(Leaf1(l2.a)),
                            &l2.b,
                            Node(Leaf1(r))
                        );
                    },
                    stack.tail()
                );
            }
        }
    
        std::tuple<const Ptr*, const Ptr*, const Ptr*> insertItem(
            std::function<int(const Ptr&, const Ptr&)> compare, const Ptr& x, const Ptr& a, const Ptr& b);
    
        /*!
         * Insert a value into this node, returning either the new node, or the
         * overflow (as a Node2) if it doesn't fit.
         */
        typename Node::InsertResult Node::insert(const Comparator& compare, const Ptr& x) const
        {
            if (const Leaf1* leaf1 = boost::get<Leaf1>(&n)) {
                int cmp = compare(x, leaf1->a);
                return Node::InsertResult(
                    cmp == 0 ? Node(Leaf1(x)) :
                    cmp < 0  ? Node(Leaf2(x, leaf1->a)) :
                               Node(Leaf2(leaf1->a, x))
                );
            }
            else
            if (const Leaf2* leaf2 = boost::get<Leaf2>(&n)) {
                if (compare(x, leaf2->a) == 0) {
                    return Node::InsertResult(Node(Leaf2(x, leaf2->b)));
                } else
                if (compare(x, leaf2->b) == 0) return Node::InsertResult(Node(Leaf2(leaf2->a, x))); else {
                    auto sml = insertItem(compare, x, leaf2->a, leaf2->b);
                    return Node::InsertResult(Node2(  // overflow
                        mkPtr<Node>(new Node(Leaf1(*std::get<0>(sml)))),
                        *std::get<1>(sml),
                        mkPtr<Node>(new Node(Leaf1(*std::get<2>(sml))))
                    ));
                }
            }
            else
            if (const Node2* node2 = boost::get<Node2>(&n)) {
                int cmp = compare(x, node2->a);
                if (cmp == 0) {
                    return Node::InsertResult(Node(Node2(node2->p, x, node2->q)));
                }
                else
                if (cmp < 0) {
                    auto result = ((const Node*)node2->p.value)->insert(compare, x);
                    if (const Node* newNode = boost::get<Node>(&result)) {
                        return Node::InsertResult(Node(Node2(
                            mkPtr<Node>(new Node(*newNode)),
                            node2->a,
                            node2->q
                        )));
                    }
                    else {
                        const Node2& overflow = boost::get<Node2>(result);
                        return Node::InsertResult(Node(Node3(
                            overflow.p,
                            overflow.a,  // to do: remove this copy
                            overflow.q,
                            node2->a,
                            node2->q
                        )));
                    }
                }
                else {
                    auto result = ((const Node*)node2->q.value)->insert(compare, x);
                    if (const Node* newNode = boost::get<Node>(&result)) {
                        return Node::InsertResult(Node(Node2(
                            node2->p,
                            node2->a,
                            mkPtr<Node>(new Node(*newNode))
                        )));
                    }
                    else {
                        const Node2& overflow = boost::get<Node2>(result);
                        return Node::InsertResult(Node(Node3(  // overflow
                            node2->p,
                            node2->a,
                            overflow.p,
                            overflow.a,  // to do: remove this copy
                            overflow.q
                        )));
                    }
                }
            }
            else {
                const Node3& node3 = boost::get<Node3>(n);
                if (compare(x, node3.a) == 0)
                    return Node::InsertResult(Node(Node3(node3.p, x, node3.q, node3.b, node3.r)));
                else
                if (compare(x, node3.b) == 0)
                    return Node::InsertResult(Node(Node3(node3.p, node3.a, node3.q, x, node3.r)));
                else
                if (compare(x, node3.a) < 0) {
                    auto result = ((Node*)node3.p.value)->insert(compare, x);
                    if (const Node* newNode = boost::get<Node>(&result)) {
                        return Node::InsertResult(Node(Node3(
                            mkPtr<Node>(new Node(*newNode)),
                            node3.a,
                            node3.q,
                            node3.b,
                            node3.r
                        )));
                    }
                    else {
                        const Node2& overflow = boost::get<Node2>(result);
                        return Node::InsertResult(Node2(  // overflow
                            mkPtr<Node>(new Node(overflow)),
                            node3.a,
                            mkPtr<Node>(new Node(Node2(node3.q, node3.b, node3.r)))
                        ));
                    }
                }
                else
                if (compare(x, node3.b) < 0) {
                    auto result = ((const Node*)node3.q.value)->insert(compare, x);
                    if (const Node* newNode = boost::get<Node>(&result)) {
                        return Node::InsertResult(Node(Node3(
                            node3.p,
                            node3.a,
                            mkPtr<Node>(new Node(*newNode)),
                            node3.b,
                            node3.r
                        )));
                    }
                    else {
                        const Node2& overflow = boost::get<Node2>(result);
                        return Node::InsertResult(Node2(  // overflow
                            mkPtr<Node>(new Node(Node2(node3.p, node3.a, overflow.p))),
                            overflow.a,  // to do: remove this copy
                            mkPtr<Node>(new Node(Node2(overflow.q, node3.b, node3.r)))
                        ));
                    }
                }
                else {
                    auto result = ((const Node*)node3.r.value)->insert(compare, x);
                    if (const Node* newNode = boost::get<Node>(&result)) {
                        return Node::InsertResult(Node(Node3(
                            node3.p,
                            node3.a,
                            node3.q,
                            node3.b,
                            mkPtr<Node>(new Node(*newNode))
                        )));
                    }
                    else {
                        const Node2& overflow = boost::get<Node2>(result);
                        return Node::InsertResult(Node2(  // overflow
                            mkPtr<Node>(new Node(Node2(node3.p, node3.a, node3.q))),
                            node3.b,
                            mkPtr<Node>(new Node(overflow))
                        ));
                    }
                }
            }
        }
    };
};

