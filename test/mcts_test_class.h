// Copyright (c) 2019 Julian Bernhard
// 
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.
// ========================================================

#ifndef MCTSTEST_H
#define MCTSTEST_H

#include "mcts/mcts.h"
#include "mcts/heuristics/random_heuristic.h"
#include "mcts/statistics/uct_statistic.h"
#include "test/simple_state.h"

using namespace mcts;
using namespace std;


class mcts::MctsTest
{

public:
    template< class S, class SE, class SO, class H>
    void verify_uct(const Mcts<S, SE, SO, H>& mcts, unsigned int depth) {
        std::vector<UctStatistic> expected_root_statistics = verify_uct(mcts.root_, depth);
    }

    template< class S, class H>
    std::vector<UctStatistic> verify_uct(const StageNodeSPtr<S,UctStatistic,UctStatistic,H>& start_node, unsigned int depth)
    {
            if(start_node->children_.empty())
            {
               return std::vector<UctStatistic>();
            }


            const AgentIdx num_agents = start_node->state_->get_agent_idx().size();


            std::vector<UctStatistic> expected_statistics(num_agents, UctStatistic(start_node->get_state()->get_num_actions(0)));

            // ----- RECURSIVE ESTIMATION OF QVALUES AND COUNTS downwards tree -----------------------
            for(auto it = start_node->children_.begin(); it != start_node->children_.end(); ++it) {
                std::vector<UctStatistic> expected_child_statistics = verify_uct(it->second,depth);

                // check joint actions are different
                auto it_other_child = it;
                for (std::advance(it_other_child,1); it_other_child != start_node->children_.end(); ++it_other_child) {
                    std::stringstream ss;
                    ss << "Equal joint action child-ids: "<<  it->second->id_ << " and "
                    << it_other_child->second->id_  << ", keys: " << static_cast<JointAction>(it->first) << " and " << static_cast<JointAction>(it_other_child->first);

                    EXPECT_TRUE( it->second->joint_action_ != it_other_child->second->joint_action_) << ss.str();
                }

                auto& child = it->second;
                std::vector<Reward> rewards;
                auto& joint_action = child->joint_action_;
                auto new_state =  start_node->state_->execute(joint_action, rewards);

                // ---------------------- Expected statistics calculation --------------------------
                bool is_first_child_and_not_parent_root = (it == start_node->children_.begin()) && (!start_node->is_root());
                bool is_last_children = (std::next(it) == start_node->children_.end());
                expected_statistics = expected_total_node_visits(it->second->ego_int_node_, S::ego_agent_idx, is_first_child_and_not_parent_root, expected_statistics);
                expected_statistics = expected_action_count(it->second->ego_int_node_, S::ego_agent_idx, joint_action, is_first_child_and_not_parent_root, expected_statistics);
                expected_statistics = expected_action_value(it->second->ego_int_node_, start_node->ego_int_node_,
                                 S::ego_agent_idx, joint_action, rewards, expected_statistics,
                                  action_occurence(start_node,joint_action[S::ego_agent_idx] , S::ego_agent_idx));

                for (int i = 0; i < child->other_int_nodes_.size(); ++i)  {
                    const auto child_int_node = child->other_int_nodes_[i];
                    const auto parent_int_node = start_node->other_int_nodes_[i];
                    expected_statistics = expected_total_node_visits(child_int_node, child_int_node.get_agent_idx(), is_first_child_and_not_parent_root, expected_statistics);
                    expected_statistics = expected_action_count(child_int_node, child_int_node.get_agent_idx(),joint_action, is_first_child_and_not_parent_root, expected_statistics );
                    expected_statistics = expected_action_value(child_int_node, parent_int_node, child_int_node.get_agent_idx(), joint_action, rewards, expected_statistics,
                                            action_occurence(start_node,joint_action[child_int_node.get_agent_idx()] , child_int_node.get_agent_idx()));
                }
            }

           // expected_statistics = expected_value(start_node->ego_int_node_, S::ego_agent_idx, expected_statistics);
           // for (auto it_other_int_nodes_root = start_node->other_int_nodes_.begin(); it_other_int_nodes_root != start_node->other_int_nodes_.end(); ++it_other_int_nodes_root) {
           //     expected_statistics = expected_value(*it_other_int_nodes_root, it_other_int_nodes_root->get_agent_idx(), expected_statistics);
           // }

            // --------- COMPARE RECURSIVE ESTIMATION AGAINST EXISTING BACKPROPAGATION VALUES  ------------
            compare_expected_existing(expected_statistics,start_node->ego_int_node_,start_node->id_,start_node->depth_);

            for (auto it = start_node->other_int_nodes_.begin(); it != start_node->other_int_nodes_.end(); ++it  )
            {
                compare_expected_existing(expected_statistics,*it,start_node->id_,start_node->depth_);
            }

            return expected_statistics;

    }
private:
    template< class S, class H>
    int action_occurence(const StageNodeSPtr<S,UctStatistic,UctStatistic,H>& node, const ActionIdx& action_idx, const AgentIdx & agent_idx) {
        // Counts how an agent selected an action in a state
        int count = -1;
        for(auto it = node->children_.begin(); it != node->children_.end(); ++it) {
            auto& joint_action = it->second->joint_action_;
            if (joint_action[agent_idx] == action_idx) {
                if (count == -1) {
                    count = 1;
                }
                else {
                    count++;
                }
            }
        }
        return count;
    }

    // update expected ucb_stat, this functions gets called once for each agent for each child (= number agents x number childs)
    std::vector<UctStatistic> expected_total_node_visits(const UctStatistic& child_stat,
             const AgentIdx& agent_idx, bool is_first_child_and_not_parent_root, std::vector<UctStatistic> expected_statistics) {
        expected_statistics[agent_idx].total_node_visits_ += child_stat.total_node_visits_; // total count for childs + 1 (first expansion of child_stat)
        if(is_first_child_and_not_parent_root) {
            expected_statistics[agent_idx].total_node_visits_ += 1;
        }

        return expected_statistics;
    }

    std::vector<UctStatistic> expected_action_count(const UctStatistic& child_stat, const AgentIdx& agent_idx,
         const JointAction& joint_action, bool is_first_child_and_not_parent_root,
         std::vector<UctStatistic> expected_statistics) {
        expected_statistics[agent_idx].ucb_statistics_[joint_action[agent_idx]].action_count_ +=  child_stat.total_node_visits_;

        return expected_statistics;
    }

    std::vector<UctStatistic> expected_action_value(const UctStatistic& child_stat,
             const UctStatistic& parent_stat, const AgentIdx& agent_idx, const JointAction& joint_action, std::vector<Reward> rewards,
             std::vector<UctStatistic> expected_statistics, int action_occurence) {
        auto action_ucb_parent = parent_stat.ucb_statistics_.find(joint_action[agent_idx]);
        if(action_ucb_parent ==  parent_stat.ucb_statistics_.end()) {
            throw;
        }
        //todo: Q(s,a) = (sum of rewards + discount*value_child1*n_visits_child1+ discount*value_child1*n_visits_child2)/total_action_count
        // total_action_count == n_visits_child1 + n_visits_child2
        const auto& total_action_count = action_ucb_parent->second.action_count_;
        const auto& child_action_count = child_stat.total_node_visits_;
        expected_statistics[agent_idx].ucb_statistics_[joint_action[agent_idx]].action_value_ +=
                         1/float(total_action_count) * rewards[agent_idx]
                        + 1/float(total_action_count) * child_action_count * parent_stat.k_discount_factor*child_stat.value_;

        return expected_statistics;
    }

    std::vector<UctStatistic> expected_value(const UctStatistic& stat, const AgentIdx& agent_idx, std::vector<UctStatistic> expected_statistics) {
        // add up all child node counts which may belong to different actions of the other agents
        expected_statistics[agent_idx].value_ = 0.0f;
        for (auto ucb_pair_it = stat.ucb_statistics_.begin(); ucb_pair_it != stat.ucb_statistics_.end(); ++ucb_pair_it) {
            expected_statistics[agent_idx].value_ += ucb_pair_it->second.action_value_;
        }
        expected_statistics[agent_idx].value_ /=  stat.ucb_statistics_.size();

        return expected_statistics;
    }
    


    template<class S, class Stats>
    void compare_expected_existing(const std::vector<UctStatistic>& expected_statistics,  const IntermediateNode<S,Stats>& inter_node,
                                   unsigned id, unsigned depth) {
        // Compare node visits
        const AgentIdx agent_idx = inter_node.get_agent_idx();
        auto recursive_node_visit = expected_statistics[agent_idx].total_node_visits_;
        auto existing_node_visit = inter_node.total_node_visits_;
        EXPECT_EQ(existing_node_visit, recursive_node_visit) << "Unexpected recursive node visits for node " << id << " at depth " << depth << " for agent " << (int)agent_idx;

        auto recursively_expected_value = expected_statistics[agent_idx].value_;
        double existing_value = inter_node.value_;
      //  EXPECT_EQ(existing_value, recursively_expected_value) << "Unexpected recursive value for node " << id << " at depth " << depth << " for agent " << (int)agent_idx;


        ASSERT_EQ((int)inter_node.state_.get_num_actions(agent_idx),inter_node.ucb_statistics_.size()) << "Internode state and statistic are of unequal length";
        for (auto action_it = inter_node.ucb_statistics_.begin(); action_it != inter_node.ucb_statistics_.end(); ++action_it)
        {   
            ActionIdx action_idx = action_it->first;
            
            if(expected_statistics[agent_idx].ucb_statistics_.find(action_idx) ==  expected_statistics[agent_idx].ucb_statistics_.end()) {
                std::cout << "skipping ucb statistic pair due to missing entry." << std::endl;
                continue; // UCBStatistic class initialized map for all available actions, but during search are only some of them expanded.
                        // Only the expanded actions are recursively estimated 
            }    

            auto recursively_expected_qvalue = expected_statistics[agent_idx].ucb_statistics_.at(action_idx).action_value_;
            double existing_qvalue = inter_node.ucb_statistics_.at(action_idx).action_value_;
            EXPECT_NEAR(existing_qvalue, recursively_expected_qvalue, 0.001) << "Unexpected recursive q-value for node "
                     << id << " at depth " << depth << " for agent " << (int)agent_idx <<  " and action " << (int)action_idx; 

            auto recursively_expected_count = expected_statistics[agent_idx].ucb_statistics_.at(action_idx).action_count_;
            unsigned existing_count = inter_node.ucb_statistics_.at(action_idx).action_count_; 

            EXPECT_EQ(existing_count, recursively_expected_count) << "Unexpected recursive action count for node "
                     << id << " at depth " << depth << " for agent " << (int)agent_idx  <<  " and action " << (int)action_idx;

        }
    }
};
#endif