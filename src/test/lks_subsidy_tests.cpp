// Copyright (c) 2026 The Lksc Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Golden-vector regression tests for the LKSCOIN block subsidy schedule.
// Vectors generated from LKSCOIN Core 3.3.0.0 (consensus-critical: any
// deviation from these values would fork the chain).

#include <chainparams.h>
#include <validation.h>
#include <test/test_lks.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(lks_subsidy_tests, BasicTestingSetup)

static CAmount FullSubsidy(int nPrevHeight, const Consensus::Params& p)
{
    // full miner+MN+superblock value
    return GetBlockSubsidy(0, nPrevHeight, p, false) + GetBlockSubsidy(0, nPrevHeight, p, true);
}

BOOST_AUTO_TEST_CASE(subsidy_schedule_main)
{
    const auto& p = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    struct { int height; CAmount lks; } vectors[] = {
        {0, 50000000}, {39, 50000000},          // premine window
        {40, 10000}, {8039, 10000},             // step 2
        {8040, 45}, {44319, 45},                // step 3
        {44320, 500}, {100000, 500}, {1000000, 500}, // steady state
        {48989, 100000}, {48990, 500},          // jackpot + neighbor
        {53659, 100000}, {58329, 200000}, {62999, 100000},
        {67669, 100000}, {72339, 200000}, {77009, 100000},
        {81679, 100000}, {86349, 200000}, {91019, 100000},
        {95689, 1000000},                       // grand jackpot
        {399062, 100000},                       // second jackpot cycle
    };
    for (const auto& v : vectors) {
        BOOST_CHECK_EQUAL(FullSubsidy(v.height, p), v.lks * COIN);
    }
}

BOOST_AUTO_TEST_CASE(masternode_payment_flat_80_percent)
{
    BOOST_CHECK_EQUAL(GetMasternodePayment(500000, 500 * COIN, 0), 400 * COIN);
    BOOST_CHECK_EQUAL(GetMasternodePayment(1, 100 * COIN, 0), 80 * COIN);
    // nReallocActivationHeight must have NO effect on LKS
    BOOST_CHECK_EQUAL(GetMasternodePayment(500000, 500 * COIN, 1), 400 * COIN);
}

BOOST_AUTO_TEST_SUITE_END()
