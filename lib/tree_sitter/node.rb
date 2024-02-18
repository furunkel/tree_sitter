require 'tree_sitter/core'

module TreeSitter
  class Node

    class << self
      def diff(old, new, output_equal: false)
        __diff__(old, new, output_equal)
      end
    end

    def cursor
      Tree::Cursor.new self
    end

    def to_h(byte_ranges: false, unnamed: false)
      __to_h__(byte_ranges, unnamed)
    end

    def inspect
      text = self.text&.then { %(#{_1.inspect} )}
      "#<#{self.class}: #{text}#{type} (#{byte_range.inspect})>"
    end

    def pq_profile(p, q, include_root_parents: false, raw: false, max_depth: nil)
      __pq_profile__(p, q, include_root_parents, raw, max_depth)
    end

    def tokenize(ignore_whitespace: true, ignore_comments: false)
      __tokenize__(ignore_whitespace, ignore_comments)
    end
  end
end