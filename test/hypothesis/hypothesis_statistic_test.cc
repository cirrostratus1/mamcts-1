// Copyright (c) 2019 Julian Bernhard
// 
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.
// ========================================================

#include "gtest/gtest.h"
#include <gtest/gtest_prod.h>

#define UNIT_TESTING
#define DEBUG
#define PLAN_DEBUG_INFO
#include "mcts/hypothesis/hypothesis_statistic.h"
#include "test/hypothesis/hypothesis_statistic_test_state.h"
#include "mcts/hypothesis/hypothesis_belief_tracker.h"
#include <cstdio>

using namespace std;
using namespace mcts;


std::mt19937  mcts::RandomGenerator::random_generator_;


TEST(hypothesis_statistic, backprop_hypothesis_action_selection) {
  std::unordered_map<AgentIdx, HypothesisId> current_agents_hypothesis = {
      {1,0}, {2,1}
  };

  // First iteration with hypothesis 0 for agent 1
  HypothesisStatisticTestState state(current_agents_hypothesis);
  HypothesisStatistic stat_parent(5,1); // agents 1 statistic 
  auto action_idx = stat_parent.choose_next_action(state);
  EXPECT_EQ(action_idx, 5);
  stat_parent.collect( 1, 2.3f, action_idx);

  HypothesisStatistic heuristic(5,1);
  heuristic.set_heuristic_estimate(10.0f , 20.0f);

  HypothesisStatistic stat_child(5,1);
  stat_child.update_from_heuristic(heuristic);
  stat_parent.update_statistic(stat_child);

  const auto ucb_stats =stat_parent.get_ucb_statistics();
  const auto node_counts = stat_parent.get_total_node_visits();

  EXPECT_NEAR(ucb_stats.at(0).at(action_idx).action_ego_cost_, 2.3f
                    +mcts::MctsParameters::DISCOUNT_FACTOR*20.0f, 0.001);
  EXPECT_EQ(ucb_stats.at(0).at(action_idx).action_count_, 1);
  EXPECT_EQ(node_counts.at(0), 1);

  // Second update with hypothesis 0 for agent 1, action is the same as only one action available
  HypothesisStatistic heuristic2(5,1);
  heuristic2.set_heuristic_estimate(15.0f , 24.5f);

  HypothesisStatistic stat_child2(5,1);
  stat_child2.update_from_heuristic(heuristic2);
  auto action_idx2 = stat_parent.choose_next_action(state);
  EXPECT_EQ(action_idx2, 5);
  stat_parent.collect( 1, 4.3f, action_idx2);
  stat_parent.update_statistic(stat_child2);

  const auto ucb_stats2 =stat_parent.get_ucb_statistics();
  const auto node_counts2 = stat_parent.get_total_node_visits();

  EXPECT_NEAR(ucb_stats2.at(0).at(action_idx2).action_ego_cost_, (2.3f+4.3
                    +mcts::MctsParameters::DISCOUNT_FACTOR*20.0f+
                    mcts::MctsParameters::DISCOUNT_FACTOR*24.5f)/2, 0.001);
  EXPECT_EQ(ucb_stats2.at(0).at(action_idx2).action_count_, 2);
  EXPECT_EQ(node_counts2.at(0), 2);

  // Third update with changed actions hypothesis 0 for agent 1
  state.change_actions();
  HypothesisStatistic heuristic3(5,1);
  heuristic3.set_heuristic_estimate(15.0f , 450.5f);

  HypothesisStatistic stat_child3(5,1);
  stat_child3.update_from_heuristic(heuristic3);
  auto action_idx3 = stat_parent.choose_next_action(state);
  EXPECT_EQ(action_idx3, 3);
  stat_parent.collect( -1, 1000.3f, action_idx3);
  stat_parent.update_statistic(stat_child3);

  const auto ucb_stats3 =stat_parent.get_ucb_statistics();
  const auto node_counts3 = stat_parent.get_total_node_visits();

  EXPECT_NEAR(ucb_stats3.at(0).at(action_idx3).action_ego_cost_, 1000.3f +
                           mcts::MctsParameters::DISCOUNT_FACTOR*450.5f, 0.001);
  EXPECT_EQ(ucb_stats3.at(0).at(action_idx3).action_count_, 1);
  EXPECT_EQ(node_counts3.at(0), 3);

  // Fourth update with hypothesis 1 for agent 1 
  current_agents_hypothesis = {
      {1,1}, {2,1}
  }; //< state holds a refence to current selected hypothesis

  HypothesisStatistic heuristic4(5,1);
  heuristic4.set_heuristic_estimate(15.0f , 45.5f);

  HypothesisStatistic stat_child4(5,1);
  stat_child4.update_from_heuristic(heuristic4);
  auto action_idx4 = stat_parent.choose_next_action(state);
  EXPECT_EQ(action_idx4, 4);
  stat_parent.collect( -1, 10.3f, action_idx4);
  stat_parent.update_statistic(stat_child4);

  const auto ucb_stats4 =stat_parent.get_ucb_statistics();
  const auto node_counts4 = stat_parent.get_total_node_visits();

  EXPECT_NEAR(ucb_stats4.at(1).at(action_idx4).action_ego_cost_, 10.3f +
                           mcts::MctsParameters::DISCOUNT_FACTOR*45.5f, 0.001);
  EXPECT_EQ(ucb_stats4.at(1).at(action_idx4).action_count_, 1);
  EXPECT_EQ(node_counts4.at(1), 1);


}

TEST(hypothesis_statistic, backprop_heuristic_hyp1) {
  // Now test something for agent 2
  const std::unordered_map<AgentIdx, HypothesisId> current_agents_hypothesis = {
      {1,0}, {2,1}
  };
  HypothesisStatisticTestState state(current_agents_hypothesis);
  HypothesisStatistic stat_parent(5,2); // agents 2 statistic 
  auto action_idx = stat_parent.choose_next_action(state);
  stat_parent.collect( 1, 5.3f, action_idx);

  HypothesisStatistic heuristic(5,2);
  heuristic.set_heuristic_estimate(10.0f , 22.0f);

  HypothesisStatistic stat_child(5,2);
  stat_child.update_from_heuristic(heuristic);
  stat_parent.update_statistic(stat_child);

  const auto ucb_stats =stat_parent.get_ucb_statistics();
  const auto node_counts = stat_parent.get_total_node_visits();

  EXPECT_NEAR(ucb_stats.at(1).at(action_idx).action_ego_cost_, 5.3f
                    +mcts::MctsParameters::DISCOUNT_FACTOR*22.0f, 0.001);
  EXPECT_EQ(ucb_stats.at(1).at(action_idx).action_count_, 1);
  EXPECT_EQ(node_counts.at(1), 1);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();

}
