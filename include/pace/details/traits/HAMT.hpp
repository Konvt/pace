#ifndef PACE_HAMT
#define PACE_HAMT

#include "../utils/Backport.hpp"
#include "TypeHash.hpp"
#include "TypeList.hpp"

namespace pace {
  namespace details {
    namespace traits {
      using HAMTHash   = std::uint32_t;
      using HAMTBitmap = std::uint32_t;

      // Not found or empty.
      struct HAMTNil {};

      template<HAMTHash Hash, typename K, typename V>
      struct HAMTLeaf {};

      template<HAMTHash Hash, typename... Leaves>
      struct HAMTCollision {};

      template<HAMTBitmap Bitmap, typename... Children>
      // 5-bit slot
      struct HAMTBranch {};

      template<HAMTHash Hash, std::uint8_t Level>
      struct HAMTChunk : std::integral_constant<std::uint8_t, ( Hash >> ( Level * 5 ) ) & 0x1Fu> {};

      template<typename T, std::uint8_t Level>
      struct HAMTBit
        : std::integral_constant<HAMTBitmap,
                                 ( HAMTBitmap { 1 } << HAMTChunk<TypeHash32<T>::value, Level>::value )> {};

      template<typename Node, typename K, std::uint8_t Level = 0>
      // Node is Nil
      struct HAMTFind {
        using type = HAMTNil;
      };
      // key mismatch
      template<HAMTHash Hash, typename EK, typename K, typename V, std::uint8_t Level>
      struct HAMTFind<HAMTLeaf<Hash, EK, V>, K, Level> {
        using type = HAMTNil;
      };
      // found
      template<HAMTHash Hash, typename K, typename V, std::uint8_t Level>
      struct HAMTFind<HAMTLeaf<Hash, K, V>, K, Level> {
        using type = V;
      };
      // collision search
      template<HAMTHash H, typename... Ls, typename K, std::uint8_t Level>
      struct HAMTFind<HAMTCollision<H, Ls...>, K, Level> {
      private:
        template<typename... Leaves>
        struct Helper {
          using type = HAMTNil;
        };
        template<HAMTHash Hash, typename V, typename... Leaves>
        struct Helper<K, HAMTLeaf<Hash, K, V>, Leaves...> {
          using type = V;
        };
        template<HAMTHash Hash, typename EK, typename V, typename... Leaves>
        struct Helper<K, HAMTLeaf<Hash, EK, V>, Leaves...> : Helper<K, Leaves...> {};

      public:
        using type = typename Helper<K, Ls...>::type;
      };
      // branch select
      template<HAMTBitmap Bitmap, typename... Cs, typename K, std::uint8_t Level>
      struct HAMTFind<HAMTBranch<Bitmap, Cs...>, K, Level> {
      private:
        template<HAMTBitmap Eigen, std::uint8_t Next>
        struct Helper
          : HAMTFind<TypeAt_t<utils::popcount( ( Bitmap ) & ( HAMTBit<K, Level>::value - 1 ) ), Cs...>,
                     K,
                     Next> {};
        template<std::uint8_t Next>
        struct Helper<0, Next> {
          using type = HAMTNil;
        };

      public:
        using type = typename Helper<Bitmap & HAMTBit<K, Level>::value, Level + 1>::type;
      };

      template<typename Node, typename K, std::uint8_t Level = 0>
      // Node is Nil
      struct HAMTTake {
        using type     = Node;
        using out_type = HAMTNil;
      };
      // key mismatch
      template<HAMTHash Hash, typename EK, typename K, typename V, std::uint8_t Level>
      struct HAMTTake<HAMTLeaf<Hash, EK, V>, K, Level> {
        using type     = HAMTLeaf<Hash, EK, V>;
        using out_type = HAMTNil;
      };
      // found and remove
      template<HAMTHash Hash, typename K, typename V, std::uint8_t Level>
      struct HAMTTake<HAMTLeaf<Hash, K, V>, K, Level> {
        using type     = HAMTNil;
        using out_type = V;
      };
      // collision search
      template<HAMTHash H, typename... Ls, typename K, std::uint8_t Level>
      struct HAMTTake<HAMTCollision<H, Ls...>, K, Level> {
      private:
        template<HAMTHash Hash, typename Leaves>
        struct Helper {
          using type     = HAMTCollision<H, Ls...>;
          using out_type = HAMTNil;
        };
        template<typename V, typename... Leaves>
        struct Helper<H, TypeList<HAMTLeaf<H, K, V>, Leaves...>> {
          using type     = TypeList<Leaves...>;
          using out_type = V;
        };
        template<typename EK, typename V, typename... Leaves>
        struct Helper<H, TypeList<HAMTLeaf<H, EK, V>, Leaves...>> {
        private:
          using Result = Helper<H, TypeList<Leaves...>>;

        public:
          using type     = TpPrepend_t<typename Result::type, HAMTLeaf<H, EK, V>>;
          using out_type = typename Result::out_type;
        };

        template<typename Leaves>
        struct Rebuild {
          // match TypeList<>
          using type = HAMTNil;
        };
        template<HAMTHash Hash, typename... Leaves>
        struct Rebuild<HAMTCollision<Hash, Leaves...>> {
          // match the first overload of Helper
          using type = HAMTCollision<Hash, Leaves...>;
        };
        template<typename First, typename Second, typename... Rests>
        struct Rebuild<TypeList<First, Second, Rests...>> {
          using type = HAMTCollision<H, First, Second, Rests...>;
        };
        template<typename Leaf>
        struct Rebuild<TypeList<Leaf>> {
          using type = Leaf;
        };

        using Result = Helper<TypeHash32<K>::value, TypeList<Ls...>>;

      public:
        using type     = typename Rebuild<typename Result::type>::type;
        using out_type = typename Result::out_type;
      };
      template<HAMTBitmap Bitmap, typename... Cs, typename K, std::uint8_t Level>
      struct HAMTTake<HAMTBranch<Bitmap, Cs...>, K, Level> {
      private:
        template<HAMTBitmap Bits, typename List>
        struct Extract;
        template<HAMTBitmap Bits, typename... Ts>
        struct Extract<Bits, TypeList<Ts...>> {
          using type = HAMTBranch<Bits, Ts...>;
        };

        template<int I, typename Children, typename Result, typename Absent>
        struct Rebuild {
          static_assert( std::is_same<Result, HAMTNil>::value == false, "invalid deletion" );

          using type = typename Extract<Bitmap, TpReplace_t<Children, I, Result>>::type;
        };
        template<int I, typename Children, typename Node>
        struct Rebuild<I, Children, Node, HAMTNil> {
          static_assert( std::is_same<Node, HAMTNil>::value == false, "invalid deletion" );

          // deletion did not occur
          using type = HAMTBranch<Bitmap, Cs...>;
        };
        template<int I, typename First, typename Second, typename... Rests, typename Absent>
        struct Rebuild<I, TypeList<First, Second, Rests...>, HAMTNil, Absent> {
          using type = typename Extract<Bitmap & ~HAMTBit<K, Level>::value,
                                        typename Take<TypeList<First, Second, Rests...>, I>::type>::type;
        };
        template<typename Leaf, typename Absent>
        struct Rebuild<0, TypeList<Leaf>, HAMTNil, Absent> {
          using type = HAMTNil;
        };

        static constexpr auto index_value = utils::popcount( ( Bitmap ) & ( HAMTBit<K, Level>::value - 1 ) );

        template<HAMTBitmap Eigen, HAMTBitmap Bits, int I>
        struct Helper : HAMTTake<TypeAt_t<I, Cs...>, K, Level + 1> {};
        template<HAMTBitmap Bits, int I>
        struct Helper<0, Bits, I> {
          using type     = HAMTBranch<Bits, Cs...>;
          using out_type = HAMTNil;
        };

        using Result = Helper<Bitmap & HAMTBit<K, Level>::value, Bitmap, index_value>;

      public:
        using type =
          typename Rebuild<index_value, TypeList<Cs...>, typename Result::type, typename Result::out_type>::
            type;
        using out_type = typename Result::out_type;
      };

      template<typename Node, typename K, typename V, std::uint8_t Level = 0>
      struct HAMTInsert {
        // Node is Nil
        using type = HAMTLeaf<TypeHash32<K>::value, K, V>;
      };
      // update an existing entry
      template<HAMTHash Hash, typename OldV, typename K, typename NewV, std::uint8_t Level>
      struct HAMTInsert<HAMTLeaf<Hash, K, OldV>, K, NewV, Level> {
        using type = HAMTLeaf<Hash, K, NewV>;
      };
      // path collision
      template<HAMTHash H, typename K1, typename V1, typename K2, typename V2, std::uint8_t Level>
      struct HAMTInsert<HAMTLeaf<H, K1, V1>, K2, V2, Level> {
      private:
        template<bool Overlap, typename NewK, std::uint8_t Current>
        struct Helper {
          static_assert( Current < 7, "invalid level" );

          using type = HAMTBranch<( HAMTBitmap { 1 } << HAMTChunk<H, Current>::value ),
                                  typename Helper<HAMTChunk<H, Current + 1>::value
                                                    == HAMTChunk<TypeHash32<NewK>::value, Current + 1>::value,
                                                  NewK,
                                                  Current + 1>::type>;
        };
        // hash collision
        template<typename NewK>
        struct Helper<true, NewK, 6> {
          using type = HAMTCollision<H, HAMTLeaf<H, K1, V1>, HAMTLeaf<H, NewK, V2>>;
        };
        template<typename NewK, std::uint8_t Current>
        struct Helper<false, NewK, Current> {
        private:
          static constexpr auto bitmap =
            ( HAMTBitmap { 1 } << HAMTChunk<H, Current>::value ) | HAMTBit<NewK, Current>::value;

          template<bool LessThan, typename K>
          struct Choice {
            using type = HAMTBranch<bitmap, HAMTLeaf<H, K1, V1>, HAMTLeaf<TypeHash32<K>::value, K, V2>>;
          };
          template<typename K>
          struct Choice<false, K> {
            using type = HAMTBranch<bitmap, HAMTLeaf<TypeHash32<K>::value, K, V2>, HAMTLeaf<H, K1, V1>>;
          };

        public:
          using type = typename Choice<( HAMTChunk<H, Current>::value
                                         < HAMTChunk<TypeHash32<NewK>::value, Current>::value ),
                                       NewK>::type;
        };

      public:
        using type =
          typename Helper<HAMTChunk<H, Level>::value == HAMTChunk<TypeHash32<K2>::value, Level>::value,
                          K2,
                          Level>::type;
      };
      // path collision
      template<HAMTHash H, typename... Ls, typename K, typename V, std::uint8_t Level>
      struct HAMTInsert<HAMTCollision<H, Ls...>, K, V, Level> {
      private:
        template<typename NewK, typename List>
        struct Merge {
          using type = TypeList<HAMTLeaf<H, NewK, V>>;
        };
        template<typename NewK, typename OldV, typename... Rests>
        struct Merge<NewK, TypeList<HAMTLeaf<H, NewK, OldV>, Rests...>> {
          using type = TypeList<HAMTLeaf<H, NewK, V>, Rests...>;
        };
        template<typename NewK, typename First, typename... Rests>
        struct Merge<NewK, TypeList<First, Rests...>> : TpPrepend<Merge<NewK, TypeList<Rests...>>, First> {};

        template<typename List>
        struct Extract;
        template<typename... Ts>
        struct Extract<TypeList<Ts...>> {
          using type = HAMTCollision<H, Ts...>;
        };

        template<bool Overlap, typename NewK, std::uint8_t Current>
        struct Helper {
          static_assert( Current < 7, "invalid level" );

          using type = HAMTBranch<( HAMTBitmap { 1 } << HAMTChunk<H, Current>::value ),
                                  typename Helper<HAMTChunk<H, Current + 1>::value
                                                    == HAMTChunk<TypeHash32<NewK>::value, Current + 1>::value,
                                                  NewK,
                                                  Current + 1>::type>;
        };
        // hash collision
        template<typename NewK>
        struct Helper<true, NewK, 6> {
          using type = typename Extract<typename Merge<NewK, TypeList<Ls...>>::type>::type;
        };
        template<typename NewK, std::uint8_t Current>
        struct Helper<false, NewK, Current> {
        private:
          static constexpr auto bitmap =
            ( HAMTBitmap { 1 } << HAMTChunk<H, Current>::value ) | HAMTBit<NewK, Current>::value;

          template<bool LessThan, typename Key>
          struct Choice {
            using type =
              HAMTBranch<bitmap, HAMTCollision<H, Ls...>, HAMTLeaf<TypeHash32<Key>::value, Key, V>>;
          };
          template<typename Key>
          struct Choice<false, Key> {
            using type =
              HAMTBranch<bitmap, HAMTLeaf<TypeHash32<Key>::value, Key, V>, HAMTCollision<H, Ls...>>;
          };

        public:
          using type = typename Choice<( HAMTChunk<H, Current>::value
                                         < HAMTChunk<TypeHash32<NewK>::value, Current>::value ),
                                       NewK>::type;
        };

      public:
        using type =
          typename Helper<HAMTChunk<H, Level>::value == HAMTChunk<TypeHash32<K>::value, Level>::value,
                          K,
                          Level>::type;
      };
      template<HAMTBitmap Bitmap, typename... Cs, typename K, typename V, std::uint8_t Level>
      struct HAMTInsert<HAMTBranch<Bitmap, Cs...>, K, V, Level> {
      private:
        template<HAMTBitmap Bits, typename List>
        struct Extract;
        template<HAMTBitmap Bits, typename... Ts>
        struct Extract<Bits, TypeList<Ts...>> {
          using type = HAMTBranch<Bits, Ts...>;
        };

        template<HAMTBitmap Eigen, typename NewK, int I>
        struct Helper
          : Extract<Bitmap,
                    TpReplace_t<TypeList<Cs...>,
                                I,
                                typename HAMTInsert<TypeAt_t<I, Cs...>, NewK, V, Level + 1>::type>> {};
        template<typename NewK, int I>
        struct Helper<0, NewK, I>
          : Extract<Bitmap | HAMTBit<K, Level>::value,
                    TpInsertAt_t<TypeList<Cs...>, I, HAMTLeaf<TypeHash32<NewK>::value, NewK, V>>> {};

      public:
        using type = typename Helper<Bitmap & HAMTBit<K, Level>::value,
                                     K,
                                     utils::popcount( ( Bitmap ) & ( HAMTBit<K, Level>::value - 1 ) )>::type;
      };

      using HAMT = HAMTNil;

      template<typename Tree, typename K>
      using HAMTFind_t = typename HAMTFind<Tree, K>::type;
      template<typename Tree, typename K>
      using HAMTContains = Not<std::is_same<HAMTFind_t<Tree, K>, HAMTNil>>;

      template<typename Tree, typename K>
      using HAMTErase_t = typename HAMTTake<Tree, K>::type;
      template<typename Tree, typename K>
      using HAMTTakeout_t = typename HAMTTake<Tree, K>::out_type;

      template<typename Tree, typename K, typename V>
      using HAMTInsert_t = typename HAMTInsert<Tree, K, V>::type;
    } // namespace traits
  } // namespace details
} // namespace pace

#endif
