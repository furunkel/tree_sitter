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

    def tokenize(whitespace: false)
      __tokenize__(whitespace)
    end
  end
end