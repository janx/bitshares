#include <bts/blockchain/balance_record.hpp>

namespace bts { namespace blockchain {

   balance_record::balance_record( const address& owner, const asset& balance_arg, slate_id_type delegate_id )
   {
      balance =  balance_arg.amount;
      condition = withdraw_condition( withdraw_with_signature( owner ), balance_arg.asset_id, delegate_id );
   }

   address balance_record::owner()const
   {
      if( condition.type == withdraw_signature_type )
         return condition.as<withdraw_with_signature>().owner;
      if( condition.type == withdraw_vesting_type )
         return condition.as<withdraw_vesting>().owner;
      return address();
   }

   asset balance_record::get_spendable_balance( const time_point_sec& at_time )const
   {
       switch( withdraw_condition_types( condition.type ) )
       {
           case withdraw_signature_type:
           {
               return asset( balance, condition.asset_id );
           }
           case withdraw_vesting_type:
           {
               const withdraw_vesting vesting_condition = condition.as<withdraw_vesting>();

               // First calculate max that could be claimed assuming no prior withdrawals
               share_type max_claimable = 0;
               if( at_time >= vesting_condition.start_time + vesting_condition.duration )
               {
                   max_claimable = vesting_condition.original_balance;
               }
               else if( at_time > vesting_condition.start_time )
               {
                   const auto elapsed_time = (at_time - vesting_condition.start_time).to_seconds();
                   FC_ASSERT( elapsed_time > 0 && elapsed_time < vesting_condition.duration );
                   max_claimable = (vesting_condition.original_balance * elapsed_time) / vesting_condition.duration;
                   FC_ASSERT( max_claimable > 0 && max_claimable < vesting_condition.original_balance );
               }

               const share_type claimed_so_far = vesting_condition.original_balance - balance;
               FC_ASSERT( claimed_so_far >= 0 && claimed_so_far <= vesting_condition.original_balance );

               const share_type spendable_balance = max_claimable - claimed_so_far;
               FC_ASSERT( spendable_balance >= 0 && spendable_balance <= vesting_condition.original_balance );

               return asset( spendable_balance, condition.asset_id );
           }
           default:
           {
               FC_ASSERT( !"Unsupported withdraw condition!" );
           }
       }
   }

} } // bts::blockchain
