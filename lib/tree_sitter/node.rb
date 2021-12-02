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

    def inspect
      self.text || "<empty>"
    end
  end
end